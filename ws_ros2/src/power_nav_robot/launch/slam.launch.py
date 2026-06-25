import os
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    slam_toolbox = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            os.path.join(get_package_share_directory('slam_toolbox'), 'launch', 'online_async_launch.py')
        ]),
        launch_arguments={
            'slam_params_file': os.path.join(
                get_package_share_directory('power_nav_robot'), 
                'config', 'slam_params.yaml' 
            ),
            'use_sim_time': 'true'
        }.items()
    )
    
    return LaunchDescription([
        slam_toolbox
    ])