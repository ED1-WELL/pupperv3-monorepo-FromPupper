#!/usr/bin/env python3
"""
Leg Lift Controller for Pupper V3

Lifts the front-right leg until parallel with the ground while the robot
balances on front-left, back-right, and back-left legs.

Sequence:
  1. INIT  (init_duration s):  Smoothly move from current pose to walking default
  2. STAND (stand_duration s): Hold at walking default to stabilize
  3. LIFT  (lift_duration s):  Smoothly raise front-right leg to horizontal
  4. HOLD:                     Hold lifted position indefinitely

To activate: press △ on PS5 (enables forward controllers), then run this node.
"""

import numpy as np
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
from std_msgs.msg import Float64MultiArray, Empty
from enum import Enum, auto


class Phase(Enum):
    WAITING = auto()  # Waiting for first /joint_states message
    INIT    = auto()  # Interpolating from current pose to walking default
    STAND   = auto()  # Holding walking default briefly to stabilize
    LIFT    = auto()  # Raising front-right leg
    HOLD    = auto()  # Holding lifted position indefinitely


class LegLiftController(Node):
    """Lifts the front-right leg while balancing on the other three."""

    ACTION_SIZE = 12

    # Joint index groupings
    FR_INDICES      = [0, 1, 2]                   # Front-right: _1, _2, _3
    SUPPORT_INDICES = [3, 4, 5, 6, 7, 8, 9, 10, 11]  # FL + BR + BL

    def __init__(self):
        super().__init__("leg_lift_controller")

        # ==================== Parameters ====================
        self.declare_parameter(
            "joint_names",
            [
                "leg_front_r_1", "leg_front_r_2", "leg_front_r_3",
                "leg_front_l_1", "leg_front_l_2", "leg_front_l_3",
                "leg_back_r_1",  "leg_back_r_2",  "leg_back_r_3",
                "leg_back_l_1",  "leg_back_l_2",  "leg_back_l_3",
            ],
        )

        # Standard walking default — the robot stands here before the lift
        self.declare_parameter(
            "default_joint_pos",
            [0.26, 0.0, -0.52, -0.26, 0.0, 0.52, 0.26, 0.0, -0.52, -0.26, 0.0, 0.52],
        )

        # Target angles for the front-right leg when parallel to the ground.
        #   _1 (hip ab/ad): keep neutral abduction
        #   _2 (upper leg): rotate forward/upward until leg is horizontal (~1.5 rad)
        #   _3 (lower leg): extend slightly forward so the whole leg is roughly flat
        # *** Tune these if the leg overshoots or doesn't reach horizontal ***
        self.declare_parameter("fr_lift_pos_1", 0.26)   # hip  — unchanged from default
        self.declare_parameter("fr_lift_pos_2", 1.5)    # upper leg — ~85° upward
        self.declare_parameter("fr_lift_pos_3", 0.5)    # lower leg — slight forward extension

        # Support leg gains — firm hold so three legs don't sag
        self.declare_parameter("support_kp", 7.5)
        self.declare_parameter("support_kd", 0.35)

        # Lifted leg gains — slightly softer for a smooth, controlled motion
        self.declare_parameter("lift_kp", 5.0)
        self.declare_parameter("lift_kd", 0.3)

        # Phase durations (seconds)
        self.declare_parameter("init_duration",  3.0)  # time to reach default pose
        self.declare_parameter("stand_duration", 1.0)  # pause before lifting
        self.declare_parameter("lift_duration",  5.0)  # time to complete the lift

        self.declare_parameter("control_rate", 50.0)   # Hz

        # ==================== Get Parameters ====================
        self.joint_names      = self.get_parameter("joint_names").value
        self.default_joint_pos = np.array(self.get_parameter("default_joint_pos").value)

        fr1 = self.get_parameter("fr_lift_pos_1").value
        fr2 = self.get_parameter("fr_lift_pos_2").value
        fr3 = self.get_parameter("fr_lift_pos_3").value

        # Full 12-joint lifted target — only FR indices differ from default
        self.lifted_joint_pos = self.default_joint_pos.copy()
        self.lifted_joint_pos[0] = fr1
        self.lifted_joint_pos[1] = fr2
        self.lifted_joint_pos[2] = fr3

        self.support_kp = self.get_parameter("support_kp").value
        self.support_kd = self.get_parameter("support_kd").value
        self.lift_kp    = self.get_parameter("lift_kp").value
        self.lift_kd    = self.get_parameter("lift_kd").value

        self.init_duration  = self.get_parameter("init_duration").value
        self.stand_duration = self.get_parameter("stand_duration").value
        self.lift_duration  = self.get_parameter("lift_duration").value
        control_rate        = self.get_parameter("control_rate").value

        # ==================== State ====================
        self.phase            = Phase.WAITING
        self.phase_start_time = None
        self.init_positions   = None  # pose snapshot when INIT begins
        self.lift_start_pos   = None  # pose snapshot when LIFT begins

        self.current_joint_positions = None
        self.estop_active = False

        # ==================== Publishers ====================
        self.position_pub = self.create_publisher(
            Float64MultiArray, "/forward_position_controller/commands", 10
        )
        self.kp_pub = self.create_publisher(
            Float64MultiArray, "/forward_kp_controller/commands", 10
        )
        self.kd_pub = self.create_publisher(
            Float64MultiArray, "/forward_kd_controller/commands", 10
        )

        # ==================== Subscribers ====================
        self.joint_states_sub = self.create_subscription(
            JointState, "/joint_states", self.joint_states_callback, 10
        )
        self.estop_sub = self.create_subscription(
            Empty, "/emergency_stop", self.estop_callback, 10
        )

        # ==================== Timer ====================
        self.control_timer = self.create_timer(1.0 / control_rate, self.control_loop)

        self.get_logger().info("Leg Lift Controller initialized")
        self.get_logger().info(f"  FR lift target: [_1={fr1:.2f}, _2={fr2:.2f}, _3={fr3:.2f}]")
        self.get_logger().info(f"  Support gains:  kp={self.support_kp}, kd={self.support_kd}")
        self.get_logger().info(f"  Lift gains:     kp={self.lift_kp}, kd={self.lift_kd}")
        self.get_logger().info(f"  Durations:  init={self.init_duration}s  stand={self.stand_duration}s  lift={self.lift_duration}s")
        self.get_logger().info("Waiting for /joint_states...")

    # ------------------------------------------------------------------
    # Callbacks
    # ------------------------------------------------------------------

    def joint_states_callback(self, msg: JointState):
        joint_map = {name: pos for name, pos in zip(msg.name, msg.position)}
        self.current_joint_positions = np.array(
            [joint_map.get(name, 0.0) for name in self.joint_names]
        )

        # Transition out of WAITING on the first received message
        if self.phase == Phase.WAITING:
            self.phase = Phase.INIT
            self.phase_start_time = self.get_clock().now().nanoseconds / 1e9
            self.init_positions   = self.current_joint_positions.copy()
            self.get_logger().info("INIT: Moving to walking default pose...")

    def estop_callback(self, msg: Empty):
        self.estop_active = True
        self.get_logger().warn("Emergency stop activated!")

    # ------------------------------------------------------------------
    # Control loop
    # ------------------------------------------------------------------

    def control_loop(self):
        if self.current_joint_positions is None:
            return

        if self.estop_active:
            self._publish_estop()
            return

        now     = self.get_clock().now().nanoseconds / 1e9
        elapsed = now - self.phase_start_time if self.phase_start_time is not None else 0.0

        if self.phase == Phase.INIT:
            self._run_init(elapsed)
        elif self.phase == Phase.STAND:
            self._run_stand(elapsed)
        elif self.phase == Phase.LIFT:
            self._run_lift(elapsed)
        elif self.phase == Phase.HOLD:
            self._run_hold()

    # ------------------------------------------------------------------
    # Phase implementations
    # ------------------------------------------------------------------

    def _run_init(self, elapsed: float):
        """Linearly interpolate from captured initial pose → walking default."""
        alpha  = min(elapsed / self.init_duration, 1.0)
        target = self.init_positions * (1.0 - alpha) + self.default_joint_pos * alpha

        kp = np.full(self.ACTION_SIZE, self.support_kp)
        kd = np.full(self.ACTION_SIZE, self.support_kd)
        self._publish(target, kp, kd)

        if alpha >= 1.0:
            self.phase = Phase.STAND
            self.phase_start_time = self.get_clock().now().nanoseconds / 1e9
            self.get_logger().info("STAND: Stabilizing at default pose...")

    def _run_stand(self, elapsed: float):
        """Hold walking default pose briefly before starting the lift."""
        kp = np.full(self.ACTION_SIZE, self.support_kp)
        kd = np.full(self.ACTION_SIZE, self.support_kd)
        self._publish(self.default_joint_pos, kp, kd)

        if elapsed >= self.stand_duration:
            self.phase = Phase.LIFT
            self.phase_start_time = self.get_clock().now().nanoseconds / 1e9
            self.lift_start_pos   = self.current_joint_positions.copy()
            self.get_logger().info("LIFT: Raising front-right leg to horizontal...")

    def _run_lift(self, elapsed: float):
        """Smoothly raise FR leg from its current pose to the lifted target."""
        alpha  = min(elapsed / self.lift_duration, 1.0)
        target = self.default_joint_pos.copy()

        # Interpolate only the FR joints; support legs stay at default
        for i in self.FR_INDICES:
            target[i] = (
                self.lift_start_pos[i] * (1.0 - alpha)
                + self.lifted_joint_pos[i] * alpha
            )

        kp = np.full(self.ACTION_SIZE, self.support_kp)
        kd = np.full(self.ACTION_SIZE, self.support_kd)
        for i in self.FR_INDICES:
            kp[i] = self.lift_kp
            kd[i] = self.lift_kd

        self._publish(target, kp, kd)

        if alpha >= 1.0:
            self.phase = Phase.HOLD
            self.get_logger().info(
                "HOLD: Front-right leg at target. Holding until node is stopped."
            )

    def _run_hold(self):
        """Hold lifted pose indefinitely."""
        kp = np.full(self.ACTION_SIZE, self.support_kp)
        kd = np.full(self.ACTION_SIZE, self.support_kd)
        for i in self.FR_INDICES:
            kp[i] = self.lift_kp
            kd[i] = self.lift_kd

        self._publish(self.lifted_joint_pos, kp, kd)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _publish_estop(self):
        zeros = np.zeros(self.ACTION_SIZE)
        kp    = np.zeros(self.ACTION_SIZE)
        kd    = np.full(self.ACTION_SIZE, 0.1)  # light damping
        self._publish(zeros, kp, kd)

    def _publish(self, positions: np.ndarray, kps: np.ndarray, kds: np.ndarray):
        pos_msg      = Float64MultiArray()
        pos_msg.data = positions.tolist()
        self.position_pub.publish(pos_msg)

        kp_msg      = Float64MultiArray()
        kp_msg.data = kps.tolist()
        self.kp_pub.publish(kp_msg)

        kd_msg      = Float64MultiArray()
        kd_msg.data = kds.tolist()
        self.kd_pub.publish(kd_msg)


def main(args=None):
    rclpy.init(args=args)
    try:
        node = LegLiftController()
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
