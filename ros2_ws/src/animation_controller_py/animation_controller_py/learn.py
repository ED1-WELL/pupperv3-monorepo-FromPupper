"""
Learning script
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
