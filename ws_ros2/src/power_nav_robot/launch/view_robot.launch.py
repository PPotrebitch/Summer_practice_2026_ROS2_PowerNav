import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import Command
from ament_index_python.packages import get_package_share_directory
from launch_ros.parameter_descriptions import ParameterValue 

def generate_launch_description():
    pkg = get_package_share_directory('power_nav_robot')
    robot_desc = Command(['xacro ', os.path.join(pkg, 'urdf', 'robot_model.xacro')])

    return LaunchDescription([
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{
                # === ОБОРАЧИВАЕМ В ParameterValue, ЧТОБЫ ROS НЕ ПЫТАЛСЯ ЧИТАТЬ ЭТО КАК YAML ===
                'robot_description': ParameterValue(robot_desc, value_type=str),
                'use_sim_time': False
            }]
        ),
        Node(
            package='joint_state_publisher_gui',
            executable='joint_state_publisher_gui'
        ),
        Node(
            package='rviz2',
            executable='rviz2',
            arguments=['-d', os.path.join(pkg, 'config', 'rviz.rviz')]
        )
    ])