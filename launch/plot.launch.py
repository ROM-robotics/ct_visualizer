from launch import LaunchDescription
from launch_ros.actions import Node
import os
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    layout_path = os.path.join(
        get_package_share_directory('ct_visualizer'),
        'layout',
        'runner_follower_gain_layout.xml'
    )

    return LaunchDescription([
        Node(
            package='plotjuggler',
            executable='plotjuggler',
            name='plotjuggler',
            arguments=['--layout', layout_path],
            output='screen'
        )
    ])