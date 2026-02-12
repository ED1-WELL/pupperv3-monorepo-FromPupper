#!/usr/bin/env python3
"""
Wheeled PD Controller for Pupper V3

A simple PD controller for the wheeled Pupper robot that:
- Holds legs in a fixed standing position using PD control
- Controls wheels for differential drive based on cmd_vel commands

This provides a simple, tunable alternative to RL policies for driving.
"""

import numpy as np
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from sensor_msgs.msg import JointState
from std_msgs.msg import Float64MultiArray, Empty


class WheeledPDController(Node):
    """PD controller for wheeled Pupper robot."""

    # Joint indices (matching config.yaml order)
    # Legs: positions 0,1 (front-r), 3,4 (front-l), 6,7 (back-r), 9,10 (back-l) 
    # Wheels: positions 2 (front-r), 5 (front-l), 8 (back-r), 11 (back-l)
    LEG_INDICES = [0, 1, 3, 4, 6, 7, 9, 10]  # 8 leg joints
    WHEEL_INDICES = [2, 5, 8, 11]  # 4 wheel joints (FR, FL, BR, BL)
    ACTION_SIZE = 12

    def __init__(self):
        super().__init__("wheeled_pd_controller")

        # ==================== Parameters ====================
        # Joint names (must match hardware interface)
        self.declare_parameter(
            "joint_names",
            [
                "leg_front_r_1", "leg_front_r_2", "leg_front_r_3",
                "leg_front_l_1", "leg_front_l_2", "leg_front_l_3",
                "leg_back_r_1", "leg_back_r_2", "leg_back_r_3",
                "leg_back_l_1", "leg_back_l_2", "leg_back_l_3",
            ],
        )

        # Default standing pose (legs only - wheels are velocity controlled)
        # Format: [FR1, FR2, FR3(wheel), FL1, FL2, FL3(wheel), BR1, BR2, BR3(wheel), BL1, BL2, BL3(wheel)]
        self.declare_parameter(
            "default_joint_pos",
            [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
        )

        # Leg PD gains
        self.declare_parameter("leg_kp", 7.5)
        self.declare_parameter("leg_kd", 0.25)

        # Wheel velocity gain (D only - no position term for velocity control)
        self.declare_parameter("wheel_kd", 1.0)

        # Robot geometry for differential drive
        self.declare_parameter("wheel_separation", 0.167)  # meters (left-right)
        self.declare_parameter("wheel_radius", 0.04445)  # meters

        # Safety limits
        self.declare_parameter("max_wheel_velocity", 10.0)  # rad/s

        # Control rate
        self.declare_parameter("control_rate", 50.0)  # Hz

        # Initialization parameters
        self.declare_parameter("init_duration", 2.0)  # seconds to reach standing pose
        self.declare_parameter("init_kp", 7.5)
        self.declare_parameter("init_kd", 0.25)

        # ==================== Get Parameters ====================
        self.joint_names = self.get_parameter("joint_names").value
        self.default_joint_pos = np.array(self.get_parameter("default_joint_pos").value)
        
        self.leg_kp = self.get_parameter("leg_kp").value
        self.leg_kd = self.get_parameter("leg_kd").value
        self.wheel_kd = self.get_parameter("wheel_kd").value
        
        self.wheel_separation = self.get_parameter("wheel_separation").value
        self.wheel_radius = self.get_parameter("wheel_radius").value
        self.max_wheel_velocity = self.get_parameter("max_wheel_velocity").value
        
        control_rate = self.get_parameter("control_rate").value
        self.init_duration = self.get_parameter("init_duration").value
        self.init_kp = self.get_parameter("init_kp").value
        self.init_kd = self.get_parameter("init_kd").value

        # Validate parameters
        if len(self.joint_names) != self.ACTION_SIZE:
            raise ValueError(f"joint_names must have {self.ACTION_SIZE} elements")
        if len(self.default_joint_pos) != self.ACTION_SIZE:
            raise ValueError(f"default_joint_pos must have {self.ACTION_SIZE} elements")

        # ==================== State Variables ====================
        self.cmd_linear_x = 0.0
        self.cmd_angular_z = 0.0
        self.estop_active = False
        
        # Initialization state
        self.init_start_time = None
        self.init_positions = None
        self.current_joint_positions = None
        self.is_initialized = False

        # ==================== Publishers ====================
        # Forward command controllers (same as animation controller uses)
        self.position_pub = self.create_publisher(
            Float64MultiArray, "/forward_position_controller/commands", 10
        )
        self.kp_pub = self.create_publisher(
            Float64MultiArray, "/forward_kp_controller/commands", 10
        )
        self.kd_pub = self.create_publisher(
            Float64MultiArray, "/forward_kd_controller/commands", 10
        )
        # Velocity controller for wheels
        self.velocity_pub = self.create_publisher(
            Float64MultiArray, "/forward_velocity_controller/commands", 10
        )

        # ==================== Subscribers ====================
        # Velocity commands from joystick
        self.cmd_vel_sub = self.create_subscription(
            Twist, "/cmd_vel", self.cmd_vel_callback, 10
        )

        # Joint states for initialization
        self.joint_states_sub = self.create_subscription(
            JointState, "/joint_states", self.joint_states_callback, 10
        )

        # Emergency stop
        self.estop_sub = self.create_subscription(
            Empty, "/emergency_stop", self.estop_callback, 10
        )

        # ==================== Control Timer ====================
        self.control_timer = self.create_timer(1.0 / control_rate, self.control_loop)

        self.get_logger().info("Wheeled PD Controller initialized")
        self.get_logger().info(f"  Leg gains: kp={self.leg_kp}, kd={self.leg_kd}")
        self.get_logger().info(f"  Wheel gain: kd={self.wheel_kd}")
        self.get_logger().info(f"  Wheel geometry: separation={self.wheel_separation}m, radius={self.wheel_radius}m")
        self.get_logger().info("Waiting for joint_states to initialize...")

    def cmd_vel_callback(self, msg: Twist):
        """Handle velocity commands from joystick."""
        self.cmd_linear_x = msg.linear.x
        self.cmd_angular_z = msg.angular.z
        # Debug: log received commands (remove after debugging)
        if abs(msg.linear.x) > 0.01 or abs(msg.angular.z) > 0.01:
            self.get_logger().info(f"cmd_vel received: linear.x={msg.linear.x:.2f}, angular.z={msg.angular.z:.2f}")

    def joint_states_callback(self, msg: JointState):
        """Handle joint state updates."""
        try:
            # Create mapping from joint name to position
            joint_position_map = {name: pos for name, pos in zip(msg.name, msg.position)}

            # Extract positions for our joints in the correct order
            positions = []
            for joint_name in self.joint_names:
                if joint_name in joint_position_map:
                    positions.append(joint_position_map[joint_name])
                else:
                    positions.append(0.0)

            self.current_joint_positions = np.array(positions)

            # Start initialization if this is the first joint state
            if self.init_start_time is None and not self.is_initialized:
                self.init_start_time = self.get_clock().now().nanoseconds / 1e9
                self.init_positions = self.current_joint_positions.copy()
                self.get_logger().info("Starting initialization to standing pose...")

        except Exception as e:
            self.get_logger().error(f"Error processing joint_states: {e}")

    def estop_callback(self, msg: Empty):
        """Handle emergency stop."""
        self.estop_active = True
        self.get_logger().warn("Emergency stop activated!")

    def compute_wheel_velocities(self) -> tuple:
        """
        Compute wheel velocities using differential drive kinematics.
        
        Returns: (left_wheel_vel, right_wheel_vel) in rad/s
        """
        # Differential drive equations:
        # v_left  = (v - omega * L/2) / r
        # v_right = (v + omega * L/2) / r
        # where:
        #   v = linear velocity (m/s)
        #   omega = angular velocity (rad/s)
        #   L = wheel separation (m)
        #   r = wheel radius (m)

        v = self.cmd_linear_x
        omega = self.cmd_angular_z
        L = self.wheel_separation
        r = self.wheel_radius

        v_left = (v - omega * L / 2.0) / r
        v_right = (v + omega * L / 2.0) / r

        # Clamp to max velocity
        v_left = np.clip(v_left, -self.max_wheel_velocity, self.max_wheel_velocity)
        v_right = np.clip(v_right, -self.max_wheel_velocity, self.max_wheel_velocity)

        return v_left, v_right

    def control_loop(self):
        """Main control loop."""
        # Wait for joint states before doing anything
        if self.current_joint_positions is None:
            return

        # Handle emergency stop
        if self.estop_active:
            self.publish_estop()
            return

        current_time = self.get_clock().now().nanoseconds / 1e9

        # Phase 1: Initialization - smoothly move to standing pose
        if not self.is_initialized:
            if self.init_start_time is None:
                return
                
            time_since_init = current_time - self.init_start_time
            
            if time_since_init < self.init_duration:
                # Interpolate from current pose to standing pose
                alpha = time_since_init / self.init_duration
                
                # Only interpolate leg positions, wheels stay at 0 velocity
                target_positions = np.zeros(self.ACTION_SIZE)
                for i in self.LEG_INDICES:
                    target_positions[i] = (
                        self.init_positions[i] * (1.0 - alpha) + 
                        self.default_joint_pos[i] * alpha
                    )
                # Wheels: 0 velocity during init
                velocity_cmd = np.zeros(self.ACTION_SIZE)

                # Use init gains
                kp_array = np.zeros(self.ACTION_SIZE)
                kd_array = np.zeros(self.ACTION_SIZE)
                for i in self.LEG_INDICES:
                    kp_array[i] = self.init_kp
                    kd_array[i] = self.init_kd
                for i in self.WHEEL_INDICES:
                    kp_array[i] = 0.0  # No position control for wheels
                    kd_array[i] = self.init_kd

                self.publish_commands(target_positions, velocity_cmd, kp_array, kd_array)
                return
            else:
                # Initialization complete
                self.is_initialized = True
                self.get_logger().info("Initialization complete! Ready for driving commands.")

        # Phase 2: Normal operation - hold legs, control wheels
        self.run_pd_control()

    def run_pd_control(self):
        """Execute PD control for legs and velocity control for wheels."""
        # Build position command array (legs only)
        position_cmd = np.zeros(self.ACTION_SIZE)
        kp_array = np.zeros(self.ACTION_SIZE)
        kd_array = np.zeros(self.ACTION_SIZE)
        velocity_cmd = np.zeros(self.ACTION_SIZE)

        # Legs: Hold at default standing position
        for i in self.LEG_INDICES:
            position_cmd[i] = self.default_joint_pos[i]
            kp_array[i] = self.leg_kp
            kd_array[i] = self.leg_kd

        # Wheels: Velocity control using differential drive
        v_left, v_right = self.compute_wheel_velocities()

        # Wheel indices: FR=2, FL=5, BR=8, BL=11
        # Right wheels (FR, BR): indices 2, 8
        # Left wheels (FL, BL): indices 5, 11
        velocity_cmd[2] = -v_right  # FR wheel velocity (negated - right side mirrored)
        velocity_cmd[5] = v_left    # FL wheel velocity
        velocity_cmd[8] = -v_right  # BR wheel velocity (negated - right side mirrored)
        velocity_cmd[11] = v_left   # BL wheel velocity

        # Wheel gains: kp=0 (no position control), kd=wheel_kd
        for i in self.WHEEL_INDICES:
            kp_array[i] = 0.0
            kd_array[i] = self.wheel_kd

        self.publish_commands(position_cmd, velocity_cmd, kp_array, kd_array)

    def publish_estop(self):
        """Publish zero commands for emergency stop."""
        position_cmd = np.zeros(self.ACTION_SIZE)
        velocity_cmd = np.zeros(self.ACTION_SIZE)
        kp_array = np.zeros(self.ACTION_SIZE)
        kd_array = np.full(self.ACTION_SIZE, 0.1)  # Light damping

        self.publish_commands(position_cmd, velocity_cmd, kp_array, kd_array)

    def publish_commands(self, positions: np.ndarray, velocities: np.ndarray, kps: np.ndarray, kds: np.ndarray):
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
        node = WheeledPDController()
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
