from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    use_rviz = LaunchConfiguration('use_rviz')
    use_plotjuggler = LaunchConfiguration('use_plotjuggler')

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
        arguments=[
            '-d',
            PathJoinSubstitution([
                FindPackageShare('ct_visualizer'),
                'rviz',
                'vis.rviz',
            ]),
        ],
        condition=IfCondition(use_rviz),
    )

    plotjuggler_node = Node(
        package='plotjuggler',
        executable='plotjuggler',
        name='plotjuggler',
        arguments=[
            '--layout',
            PathJoinSubstitution([
                FindPackageShare('ct_visualizer'),
                'layout',
                'runner_follower_gain_layout.xml',
            ]),
        ],
        output='screen',
        condition=IfCondition(use_plotjuggler),
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_rviz',
            default_value='true',
            description='Start the RViz user interface',
        ),
        DeclareLaunchArgument(
            'use_plotjuggler',
            default_value='true',
            description='Start PlotJuggler',
        ),
        trajectory_path_publisher_node,
        drone_2_trajectory_path_publisher_node,
        rviz_node,
        plotjuggler_node,
    ])