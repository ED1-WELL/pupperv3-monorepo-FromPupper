#!/usr/bin/env python3
"""
Wheel Reshaper for Pupper V3 — Heat Memory Polymer Transformation

A timed sequence controller that uses the robot's weight to reshape
heat memory polymer wheels. The sequence:

  1. Initialize: Smoothly move to wheeled standing pose (2s)
  2. Wait phase1_hold_duration (30s) — weight compresses wheels
  3. Rotate all wheels backward 90° over rotation_duration (3s)
  4. Wait phase3_hold_duration (30s) — weight compresses new contact patch
  5. Rotate all wheels forward 45° over rotation_duration (3s)
  6. Hold final position indefinitely

Wheels use POSITION control (kp > 0) so that each rotation is precise
and the wheel holds its angle while the polymer reshapes.
"""

import numpy as np
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
from std_msgs.msg import Float64MultiArray, Empty


class WheelReshaper(Node):
    """Timed wheel reshaping sequence for heat memory polymer transformation."""

    # Joint indices (matching config.yaml order)
    LEG_INDICES = [0, 1, 3, 4, 6, 7, 9, 10]  # 8 leg joints
    WHEEL_INDICES = [2, 5, 8, 11]  # 4 wheel joints (FR, FL, BR, BL)
    RIGHT_WHEEL_INDICES = [2, 8]   # FR, BR
    LEFT_WHEEL_INDICES = [5, 11]   # FL, BL
    ACTION_SIZE = 12

    # Phases
    PHASE_INIT = 0
    PHASE_WAIT_1 = 1
    PHASE_ROTATE_BACKWARD = 2
    PHASE_WAIT_2 = 3
    PHASE_ROTATE_FORWARD = 4
    PHASE_DONE = 5

    def __init__(self):
        super().__init__("wheel_reshaper")

        # ==================== Parameters ====================
        self.declare_parameter(
            "joint_names",
            [
                "leg_front_r_1", "leg_front_r_2", "leg_front_r_3",
                "leg_front_l_1", "leg_front_l_2", "leg_front_l_3",
                "leg_back_r_1", "leg_back_r_2", "leg_back_r_3",
                "leg_back_l_1", "leg_back_l_2", "leg_back_l_3",
            ],
        )

        # Standing pose — same as wheeled_pd_controller
        self.declare_parameter(
            "default_joint_pos",
            [-0.75, 0.0, 0.0, 0.75, 0.0, 0.0, -0.75, 0.0, 0.0, 0.75, 0.0, 0.0],
        )

        # PD gains
        self.declare_parameter("leg_kp", 3.25)
        self.declare_parameter("leg_kd", 0.5)
        self.declare_parameter("wheel_kp", 3.25)
        self.declare_parameter("wheel_kd", 0.5)

        # Control rate
        self.declare_parameter("control_rate", 50.0)

        # Initialization
        self.declare_parameter("init_duration", 2.0)

        # Timing
        self.declare_parameter("phase1_hold_duration", 30.0)
        self.declare_parameter("phase3_hold_duration", 30.0)

        # Rotation angles (radians)
        self.declare_parameter("backward_angle", 1.5708)  # π/2 = 90°
        self.declare_parameter("forward_angle", 0.7854)   # π/4 = 45°

        # Duration to interpolate each rotation
        self.declare_parameter("rotation_duration", 3.0)

        # ==================== Get Parameters ====================
        self.joint_names = self.get_parameter("joint_names").value
        self.default_joint_pos = np.array(self.get_parameter("default_joint_pos").value)

        self.leg_kp = self.get_parameter("leg_kp").value
        self.leg_kd = self.get_parameter("leg_kd").value
        self.wheel_kp = self.get_parameter("wheel_kp").value
        self.wheel_kd = self.get_parameter("wheel_kd").value

        control_rate = self.get_parameter("control_rate").value
        self.init_duration = self.get_parameter("init_duration").value

        self.phase1_hold = self.get_parameter("phase1_hold_duration").value
        self.phase3_hold = self.get_parameter("phase3_hold_duration").value

        self.backward_angle = self.get_parameter("backward_angle").value
        self.forward_angle = self.get_parameter("forward_angle").value
        self.rotation_duration = self.get_parameter("rotation_duration").value

        # Validate
        if len(self.joint_names) != self.ACTION_SIZE:
            raise ValueError(f"joint_names must have {self.ACTION_SIZE} elements")
        if len(self.default_joint_pos) != self.ACTION_SIZE:
            raise ValueError(f"default_joint_pos must have {self.ACTION_SIZE} elements")

        # ==================== State ====================
        self.estop_active = False
        self.current_joint_positions = None
        self.init_start_time = None
        self.phase = self.PHASE_INIT
        self.phase_start_time = None

        # Wheel target positions (set after init when we know the starting positions)
        self.wheel_pos_after_init = np.zeros(self.ACTION_SIZE)
        self.wheel_pos_after_backward = np.zeros(self.ACTION_SIZE)
        self.wheel_pos_after_forward = np.zeros(self.ACTION_SIZE)
        self.wheel_pos_before_rotation = np.zeros(self.ACTION_SIZE)

        # For smooth init interpolation
        self.init_positions = None

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
        self.velocity_pub = self.create_publisher(
            Float64MultiArray, "/forward_velocity_controller/commands", 10
        )

        # ==================== Subscribers ====================
        self.joint_states_sub = self.create_subscription(
            JointState, "/joint_states", self.joint_states_callback, 10
        )
        self.estop_sub = self.create_subscription(
            Empty, "/emergency_stop", self.estop_callback, 10
        )

        # ==================== Control Timer ====================
        self.control_timer = self.create_timer(1.0 / control_rate, self.control_loop)

        self.get_logger().info("=" * 60)
        self.get_logger().info("Wheel Reshaper — Heat Memory Polymer Transformation")
        self.get_logger().info("=" * 60)
        self.get_logger().info(f"  Sequence: init ({self.init_duration}s)")
        self.get_logger().info(f"         → hold  ({self.phase1_hold}s)")
        self.get_logger().info(f"         → rotate backward {np.degrees(self.backward_angle):.1f}° ({self.rotation_duration}s)")
        self.get_logger().info(f"         → hold  ({self.phase3_hold}s)")
        self.get_logger().info(f"         → rotate forward {np.degrees(self.forward_angle):.1f}° ({self.rotation_duration}s)")
        self.get_logger().info(f"         → hold indefinitely")
        self.get_logger().info(f"  Leg gains:   kp={self.leg_kp}, kd={self.leg_kd}")
        self.get_logger().info(f"  Wheel gains: kp={self.wheel_kp}, kd={self.wheel_kd}")
        self.get_logger().info("Waiting for /joint_states to initialize...")

    # ==================== Callbacks ====================

    def joint_states_callback(self, msg: JointState):
        """Handle joint state updates."""
        try:
            joint_position_map = {name: pos for name, pos in zip(msg.name, msg.position)}
            positions = []
            for joint_name in self.joint_names:
                if joint_name in joint_position_map:
                    positions.append(joint_position_map[joint_name])
                else:
                    positions.append(0.0)
            self.current_joint_positions = np.array(positions)

            # Start initialization on first joint state
            if self.init_start_time is None and self.phase == self.PHASE_INIT:
                self.init_start_time = self.get_clock().now().nanoseconds / 1e9
                self.init_positions = self.current_joint_positions.copy()
                self.get_logger().info("Starting initialization — moving to standing pose...")

        except Exception as e:
            self.get_logger().error(f"Error processing joint_states: {e}")

    def estop_callback(self, msg: Empty):
        """Handle emergency stop."""
        self.estop_active = True
        self.get_logger().warn("Emergency stop activated!")

    # ==================== Control Loop ====================

    def control_loop(self):
        """Main control loop — runs through reshaping phases."""
        if self.current_joint_positions is None:
            return

        if self.estop_active:
            self.publish_estop()
            return

        current_time = self.get_clock().now().nanoseconds / 1e9

        if self.phase == self.PHASE_INIT:
            self.run_init(current_time)
        elif self.phase == self.PHASE_WAIT_1:
            self.run_hold(current_time, self.phase1_hold, "Phase 1 hold",
                          self.wheel_pos_after_init, self.PHASE_ROTATE_BACKWARD)
        elif self.phase == self.PHASE_ROTATE_BACKWARD:
            self.run_rotation(current_time, self.wheel_pos_after_init,
                              self.wheel_pos_after_backward, "Rotating backward",
                              self.PHASE_WAIT_2)
        elif self.phase == self.PHASE_WAIT_2:
            self.run_hold(current_time, self.phase3_hold, "Phase 2 hold",
                          self.wheel_pos_after_backward, self.PHASE_ROTATE_FORWARD)
        elif self.phase == self.PHASE_ROTATE_FORWARD:
            self.run_rotation(current_time, self.wheel_pos_after_backward,
                              self.wheel_pos_after_forward, "Rotating forward",
                              self.PHASE_DONE)
        elif self.phase == self.PHASE_DONE:
            self.run_final_hold()

    def run_init(self, current_time):
        """Smoothly interpolate from current pose to standing pose."""
        if self.init_start_time is None:
            return

        elapsed = current_time - self.init_start_time

        if elapsed < self.init_duration:
            alpha = elapsed / self.init_duration

            target_positions = np.zeros(self.ACTION_SIZE)
            for i in self.LEG_INDICES:
                target_positions[i] = (
                    self.init_positions[i] * (1.0 - alpha)
                    + self.default_joint_pos[i] * alpha
                )
            # Wheels: hold at their initial positions during init
            for i in self.WHEEL_INDICES:
                target_positions[i] = self.init_positions[i]

            kp_array = np.zeros(self.ACTION_SIZE)
            kd_array = np.zeros(self.ACTION_SIZE)
            for i in self.LEG_INDICES:
                kp_array[i] = self.leg_kp
                kd_array[i] = self.leg_kd
            for i in self.WHEEL_INDICES:
                kp_array[i] = self.wheel_kp
                kd_array[i] = self.wheel_kd

            velocity_cmd = np.zeros(self.ACTION_SIZE)
            self.publish_commands(target_positions, velocity_cmd, kp_array, kd_array)
        else:
            # Init complete — record wheel positions and compute targets
            self.get_logger().info("Initialization complete!")

            # Record current wheel positions as the baseline
            for i in self.WHEEL_INDICES:
                self.wheel_pos_after_init[i] = self.current_joint_positions[i]

            # Compute backward rotation targets
            # "Backward" = direction that would drive the robot backward
            # Right wheels (FR=2, BR=8): negative rotation goes backward
            # Left wheels (FL=5, BL=11): positive rotation goes backward
            self.wheel_pos_after_backward = self.wheel_pos_after_init.copy()
            for i in self.RIGHT_WHEEL_INDICES:
                self.wheel_pos_after_backward[i] = (
                    self.wheel_pos_after_init[i] - self.backward_angle
                )
            for i in self.LEFT_WHEEL_INDICES:
                self.wheel_pos_after_backward[i] = (
                    self.wheel_pos_after_init[i] + self.backward_angle
                )

            # Compute forward rotation targets (from after-backward position)
            self.wheel_pos_after_forward = self.wheel_pos_after_backward.copy()
            for i in self.RIGHT_WHEEL_INDICES:
                self.wheel_pos_after_forward[i] = (
                    self.wheel_pos_after_backward[i] + self.forward_angle
                )
            for i in self.LEFT_WHEEL_INDICES:
                self.wheel_pos_after_forward[i] = (
                    self.wheel_pos_after_backward[i] - self.forward_angle
                )

            self.transition_to_phase(self.PHASE_WAIT_1,
                                     f"Holding position for {self.phase1_hold}s...")
            # Publish the hold position immediately
            self.publish_hold(self.wheel_pos_after_init)

    def run_hold(self, current_time, duration, label, wheel_targets, next_phase):
        """Hold legs at standing pose and wheels at given targets for a duration."""
        if self.phase_start_time is None:
            return

        elapsed = current_time - self.phase_start_time
        remaining = duration - elapsed

        # Log countdown every 5 seconds
        if int(elapsed) % 5 == 0 and int(elapsed) != getattr(self, '_last_log_time', -1):
            self._last_log_time = int(elapsed)
            self.get_logger().info(f"  {label}: {remaining:.0f}s remaining...")

        if elapsed >= duration:
            self.get_logger().info(f"  {label} complete!")
            self.transition_to_phase(next_phase,
                                     f"Starting next phase...")
        else:
            self.publish_hold(wheel_targets)

    def run_rotation(self, current_time, start_wheel_pos, end_wheel_pos,
                     label, next_phase):
        """Smoothly rotate wheels from start to end positions."""
        if self.phase_start_time is None:
            return

        elapsed = current_time - self.phase_start_time

        if elapsed < self.rotation_duration:
            alpha = elapsed / self.rotation_duration
            # Smooth easing (sine ease-in-out)
            alpha = 0.5 * (1.0 - np.cos(np.pi * alpha))

            # Interpolate wheel positions
            wheel_targets = np.zeros(self.ACTION_SIZE)
            for i in self.WHEEL_INDICES:
                wheel_targets[i] = (
                    start_wheel_pos[i] * (1.0 - alpha)
                    + end_wheel_pos[i] * alpha
                )

            self.publish_hold(wheel_targets)

            # Log progress
            pct = alpha * 100.0
            if int(pct) % 25 == 0 and int(pct) != getattr(self, '_last_rot_log', -1):
                self._last_rot_log = int(pct)
                self.get_logger().info(f"  {label}: {pct:.0f}% complete")
        else:
            self.get_logger().info(f"  {label} complete!")
            self.publish_hold(end_wheel_pos)
            self.transition_to_phase(next_phase, "Rotation complete")

    def run_final_hold(self):
        """Hold final position indefinitely."""
        if not getattr(self, '_done_logged', False):
            self.get_logger().info("=" * 60)
            self.get_logger().info("RESHAPING SEQUENCE COMPLETE")
            self.get_logger().info("Holding final position. Press PS button to e-stop.")
            self.get_logger().info("=" * 60)
            self._done_logged = True
        self.publish_hold(self.wheel_pos_after_forward)

    # ==================== Helpers ====================

    def transition_to_phase(self, phase, message):
        """Move to a new phase and record the start time."""
        self.phase = phase
        self.phase_start_time = self.get_clock().now().nanoseconds / 1e9
        self.get_logger().info(message)

    def publish_hold(self, wheel_targets):
        """Publish commands to hold legs at standing pose and wheels at targets."""
        position_cmd = np.zeros(self.ACTION_SIZE)
        kp_array = np.zeros(self.ACTION_SIZE)
        kd_array = np.zeros(self.ACTION_SIZE)
        velocity_cmd = np.zeros(self.ACTION_SIZE)

        # Legs: hold at default standing pose
        for i in self.LEG_INDICES:
            position_cmd[i] = self.default_joint_pos[i]
            kp_array[i] = self.leg_kp
            kd_array[i] = self.leg_kd

        # Wheels: position control at target angle
        for i in self.WHEEL_INDICES:
            position_cmd[i] = wheel_targets[i]
            kp_array[i] = self.wheel_kp
            kd_array[i] = self.wheel_kd

        self.publish_commands(position_cmd, velocity_cmd, kp_array, kd_array)

    def publish_estop(self):
        """Publish zero commands for emergency stop."""
        position_cmd = np.zeros(self.ACTION_SIZE)
        velocity_cmd = np.zeros(self.ACTION_SIZE)
        kp_array = np.zeros(self.ACTION_SIZE)
        kd_array = np.full(self.ACTION_SIZE, 0.1)
        self.publish_commands(position_cmd, velocity_cmd, kp_array, kd_array)

    def publish_commands(self, positions, velocities, kps, kds):
        """Publish commands to forward command controllers."""
        pos_msg = Float64MultiArray()
        pos_msg.data = positions.tolist()
        self.position_pub.publish(pos_msg)

        vel_msg = Float64MultiArray()
        vel_msg.data = velocities.tolist()
        self.velocity_pub.publish(vel_msg)

        kp_msg = Float64MultiArray()
        kp_msg.data = kps.tolist()
        self.kp_pub.publish(kp_msg)

        kd_msg = Float64MultiArray()
        kd_msg.data = kds.tolist()
        self.kd_pub.publish(kd_msg)


def main(args=None):
    rclpy.init(args=args)

    try:
        node = WheelReshaper()
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
