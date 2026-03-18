"""
A minimalistic ROS 2 node to learn how subscribers work.
This node listens to the joystick commands on the /cmd_vel topic
and prints the linear and angular velocities it receives.
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist

class JoySubNode(Node):
    def __init__(self):
        # 1. Initialize the node with a name
        super().__init__("joy_listener_node")
        
        # 2. Setup a Subscriber
        # We listen to messages of type 'Twist' on the topic '/cmd_vel'.
        # Whenever a message arrives, ROS 2 will automatically run 'self.cmd_vel_callback'
        self.cmd_vel_sub = self.create_subscription(
            Twist, 
            "/cmd_vel", 
            self.cmd_vel_callback, 
            10  # This 10 is the queue size (how many messages to store if we fall behind)
        )
        
        self.get_logger().info("Joy Listener Node initialized! Waiting for joystick input on /cmd_vel...")

    # 3. Create the Callback function
    def cmd_vel_callback(self, msg: Twist):
        # We pull the linear (forward/backward) and angular (turning) speeds from the message
        linear_x = msg.linear.x
        angular_z = msg.angular.z
        
        # We only print the message if the joystick is actually being pushed
        if abs(linear_x) > 0.01 or abs(angular_z) > 0.01:
            self.get_logger().info(f"Received Command -> Forward/Back: {linear_x:.2f}, Turn: {angular_z:.2f}")


# 4. The main function to keep the node alive
def main(args=None):
    rclpy.init(args=args)            # Start up the ROS 2 system
    
    node = JoySubNode()              # Create an instance of our node
    
    try:
        rclpy.spin(node)             # 'spin' keeps the script running in an infinite loop listening for messages
    except KeyboardInterrupt:
        # This catches when you press Ctrl+C in the terminal to cleanly exit
        node.get_logger().info("Shutting down cleanly...")
    
    node.destroy_node()              # Clean up the node
    rclpy.shutdown()                 # Shut down the ROS 2 system

if __name__ == "__main__":
    main()
