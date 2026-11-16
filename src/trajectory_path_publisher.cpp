// Converts PX4 VehicleLocalPosition (NED) into an accumulated nav_msgs/Path (ENU, frame "map")
// so the commanded drone's trajectory can be viewed live in rviz2.
// Also broadcasts a TF frame (map -> <drone_name>) holding the drone's FRD body frame,
// so rviz2 can draw the FRD axes together with the frame name next to the trajectory.
#include <cmath>

#include <rclcpp/rclcpp.hpp>
#include <px4_msgs/msg/vehicle_local_position.hpp>
#include <px4_msgs/msg/vehicle_attitude.hpp>
#include <nav_msgs/msg/path.hpp>
#include <std_msgs/msg/header.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_ros/transform_broadcaster.h>

using px4_msgs::msg::VehicleAttitude;
using px4_msgs::msg::VehicleLocalPosition;

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kSqrt1_2 = 0.70710678118654752440;
}  // namespace

class TrajectoryPathPublisher : public rclcpp::Node
{
public:
    TrajectoryPathPublisher() : Node("ct_visualizer_trajectory_path_publisher")
    {
        this->declare_parameter<std::string>("px4_namespace", "/px4_1");
        this->declare_parameter<std::string>("frame_id", "map");
        this->declare_parameter<std::string>("drone_name", "drone1");
        this->declare_parameter<int>("max_path_length", 5000);

        std::string ns = this->get_parameter("px4_namespace").as_string();
        if (!ns.empty() && ns.front() != '/') ns = "/" + ns;
        while (ns.size() > 1 && ns.back() == '/') ns.pop_back();

        frame_id_ = this->get_parameter("frame_id").as_string();
        drone_name_ = this->get_parameter("drone_name").as_string();
        max_path_length_ = this->get_parameter("max_path_length").as_int();
        path_.header.frame_id = frame_id_;

        path_publisher_ = this->create_publisher<nav_msgs::msg::Path>(ns + "/trajectory_path", 10);
        tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

        rmw_qos_profile_t qos_profile = rmw_qos_profile_sensor_data;
        auto qos = rclcpp::QoS(rclcpp::QoSInitialization(qos_profile.history, 5), qos_profile);

        local_position_subscription_ = this->create_subscription<VehicleLocalPosition>(
            ns + "/fmu/out/vehicle_local_position_v1", qos,
            std::bind(&TrajectoryPathPublisher::local_position_callback, this, std::placeholders::_1));

        attitude_subscription_ = this->create_subscription<VehicleAttitude>(
            ns + "/fmu/out/vehicle_attitude", qos,
            std::bind(&TrajectoryPathPublisher::attitude_callback, this, std::placeholders::_1));

        RCLCPP_INFO(this->get_logger(),
            "=== Trajectory path publisher: namespace=%s frame_id=%s drone_name=%s ===",
            ns.c_str(), frame_id_.c_str(), drone_name_.c_str());
    }

private:
    void local_position_callback(const VehicleLocalPosition::SharedPtr msg)
    {
        geometry_msgs::msg::PoseStamped pose;
        pose.header.frame_id = frame_id_;
        pose.header.stamp = this->get_clock()->now();

        // NED -> ENU: east=y, north=x, up=-z
        pose.pose.position.x = msg->y;
        pose.pose.position.y = msg->x;
        pose.pose.position.z = -msg->z;
        pose.pose.orientation.w = 1.0;

        path_.poses.push_back(pose);
        if (max_path_length_ > 0 && static_cast<int>(path_.poses.size()) > max_path_length_) {
            path_.poses.erase(path_.poses.begin());
        }

        path_.header.stamp = pose.header.stamp;
        path_publisher_->publish(path_);

        broadcast_drone_frame(pose.header, pose.pose.position, msg);
    }

    // PX4 q: Hamilton (w, x, y, z), rotation from the FRD body frame to the NED earth frame.
    void attitude_callback(const VehicleAttitude::SharedPtr msg)
    {
        const tf2::Quaternion q_frd_to_ned(msg->q[1], msg->q[2], msg->q[3], msg->q[0]);
        attitude_enu_ = ned_to_enu_ * q_frd_to_ned;
        attitude_received_ = true;
    }

    void broadcast_drone_frame(const std_msgs::msg::Header &header,
                               const geometry_msgs::msg::Point &position,
                               const VehicleLocalPosition::SharedPtr &msg)
    {
        tf2::Quaternion orientation;
        if (attitude_received_) {
            orientation = attitude_enu_;
        } else {
            // Yaw-only FRD fallback from the NED heading until attitude arrives.
            const double heading = std::isfinite(msg->heading) ? msg->heading : 0.0;
            orientation = ned_to_enu_ * tf2::Quaternion(tf2::Vector3(0.0, 0.0, 1.0), heading);
        }

        geometry_msgs::msg::TransformStamped transform;
        transform.header = header;
        transform.child_frame_id = drone_name_;
        transform.transform.translation.x = position.x;
        transform.transform.translation.y = position.y;
        transform.transform.translation.z = position.z;
        transform.transform.rotation.x = orientation.x();
        transform.transform.rotation.y = orientation.y();
        transform.transform.rotation.z = orientation.z();
        transform.transform.rotation.w = orientation.w();
        tf_broadcaster_->sendTransform(transform);
    }

    std::string frame_id_;
    std::string drone_name_;
    int max_path_length_ = 5000;
    nav_msgs::msg::Path path_;
    // Rotation from the NED frame to the ENU frame (pi about the (1, 1, 0) axis).
    const tf2::Quaternion ned_to_enu_{tf2::Vector3(kSqrt1_2, kSqrt1_2, 0.0), kPi};
    tf2::Quaternion attitude_enu_;
    bool attitude_received_ = false;
    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr path_publisher_;
    rclcpp::Subscription<VehicleLocalPosition>::SharedPtr local_position_subscription_;
    rclcpp::Subscription<VehicleAttitude>::SharedPtr attitude_subscription_;
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<TrajectoryPathPublisher>());
    rclcpp::shutdown();
    return 0;
}
