from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    perspective_file = PathJoinSubstitution([
        FindPackageShare('ct_visualizer'),
        'config',
        'camera_dashboard.perspective',
    ])

    rqt_node = Node(
        package='rqt_gui',
        executable='rqt_gui',
        name='four_cameras_rqt',
        arguments=['--perspective-file', perspective_file],
        output='screen',
    )

    return LaunchDescription([rqt_node])
