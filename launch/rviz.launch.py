from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():

    trajectory_path_publisher_node = Node(
        package='ct_visualizer',
        executable='ct_visualizer_trajectory_path_publisher',
        name='ct_visualizer_trajectory_path_publisher',
        output='screen',
    )

    drone_2_trajectory_path_publisher_node = Node(
        package='ct_visualizer',
        executable='ct_visualizer_trajectory_path_publisher',
        name='ct_visualizer_drone_2_trajectory_path_publisher',
        output='screen',
        parameters=[{'px4_namespace': '/px4_2', 'drone_name': 'drone2'}],
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', PathJoinSubstitution([FindPackageShare('ct_visualizer'), 'rviz', 'vis.rviz'])],
    )

    return LaunchDescription([
        trajectory_path_publisher_node,
        drone_2_trajectory_path_publisher_node,
        rviz_node,
    ])

