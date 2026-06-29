import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32, Bool

class BatteryMonitor(Node):
    def __init__(self):
        super().__init__('battery_monitor')

        self.declare_parameter('initial_charge', 100.0)
        self.declare_parameter('low_threshold', 20.0)

        self.current_charge = self.get_parameter('initial_charge').value
        self.threshold = self.get_parameter('low_threshold').value  

        self.battery_level_pub = self.create_publisher(Float32, '/battery_level', 10)
        self.is_low_pub = self.create_publisher(Bool, '/is_battery_low', 10)

        self.set_battery_sub = self.create_subscription(
            Float32,
            '/set_battery',
            self.set_battery_callback,
            10
        ) 

        self.publish_battery_status()

        self.get_logger().info(f'Battery Monitor started. Charge: {self.current_charge}%')
        self.get_logger().info('Set battery level via: ros2 topic pub /set_battery std_msgs/msg/Float32 "{{data: 50.0}}"')
    
    def set_battery_callback(self, msg):
        new_charge = msg.data
        if 0.0 <= new_charge <= 100.0:
            self.current_charge = new_charge
            self.get_logger().info(f'Battery level set to: {self.current_charge}%')
            self.publish_battery_status()
        else:
            self.get_logger().warn(f'Invalid battery level: {new_charge}. Must be 0-100')

    def publish_battery_status(self):
        level_msg = Float32()
        level_msg.data = float(self.current_charge)
        self.battery_level_pub.publish(level_msg)

        is_low_msg = Bool()
        is_low_msg.data = bool(self.current_charge <= self.threshold)
        self.is_low_pub.publish(is_low_msg)

        self.get_logger().info(f'Charge: {self.current_charge:.1f}% | Is Low: {is_low_msg.data}')

def main(args=None):
    rclpy.init(args=args)
    node = BatteryMonitor()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()