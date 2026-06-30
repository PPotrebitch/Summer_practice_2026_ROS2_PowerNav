import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32, Bool
from visualization_msgs.msg import Marker
from builtin_interfaces.msg import Duration

class BatteryMonitor(Node):
    def __init__(self):
        super().__init__('battery_monitor')
        
       
        self.declare_parameter('initial_charge', 100.0)
        self.declare_parameter('low_threshold', 20.0)
        self.declare_parameter('station_x', -2.0)
        self.declare_parameter('station_y', 3.0)

        
        self.current_charge = self.get_parameter('initial_charge').value
        self.threshold = self.get_parameter('low_threshold').value
        self.station_x = self.get_parameter('station_x').value
        self.station_y = self.get_parameter('station_y').value

        
        self.battery_level_pub = self.create_publisher(Float32, '/battery_level', 10)
        self.is_low_pub = self.create_publisher(Bool, '/is_battery_low', 10)
        self.marker_pub = self.create_publisher(Marker, '/charging_station_marker', 10)

        
        self.set_battery_sub = self.create_subscription(
            Float32, '/set_battery', self.set_battery_callback, 10)

        self.publish_marker()
        self.publish_battery_status()
        
        self.get_logger().info(f'Battery Monitor started. Charge: {self.current_charge}%')
        self.get_logger().info('Set battery: ros2 topic pub /set_battery std_msgs/msg/Float32 "{data: 15.0}"')

    def publish_marker(self):
        marker = Marker()
        marker.header.frame_id = 'map'
        marker.header.stamp = self.get_clock().now().to_msg()
        marker.ns = 'charging_station'
        marker.id = 0
        marker.type = Marker.CUBE  
        marker.action = Marker.ADD
        
        marker.pose.position.x = self.station_x
        marker.pose.position.y = self.station_y
        marker.pose.position.z = 0.05  
        marker.pose.orientation.w = 1.0
        
        
        marker.scale.x = 1.0
        marker.scale.y = 1.0
        marker.scale.z = 0.1 
        
        
        marker.color.r = 0.0
        marker.color.g = 1.0
        marker.color.b = 0.0
        marker.color.a = 0.7  
        
        
        marker.lifetime = Duration(sec=0, nanosec=0)
        
        self.marker_pub.publish(marker)
        
        
        text_marker = Marker()
        text_marker.header.frame_id = 'map'
        text_marker.header.stamp = self.get_clock().now().to_msg()
        text_marker.ns = 'charging_station'
        text_marker.id = 1
        text_marker.type = Marker.TEXT_VIEW_FACING
        text_marker.action = Marker.ADD
        
        text_marker.pose.position.x = self.station_x
        text_marker.pose.position.y = self.station_y
        text_marker.pose.position.z = 0.5
        text_marker.pose.orientation.w = 1.0
        
        text_marker.scale.z = 0.2
        text_marker.color.r = 1.0
        text_marker.color.g = 1.0
        text_marker.color.b = 0.0
        text_marker.color.a = 1.0
        
        text_marker.text = 'CHARGING STATION'
        text_marker.lifetime = Duration(sec=0, nanosec=0)
        
        self.marker_pub.publish(text_marker)
        self.get_logger().info(f'Marker published at ({self.station_x}, {self.station_y})')

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