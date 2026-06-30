#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/float32.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/behavior_tree.h>
#include <chrono>
#include <thread>
#include <future>
#include <mutex>

using namespace std::chrono_literals;
using NavigateToPose = nav2_msgs::action::NavigateToPose;

// ============================================================
// УЗЕЛ 1: CheckBatteryLow (Condition)
// ============================================================
class CheckBatteryLow : public BT::ConditionNode
{
public:
  CheckBatteryLow(const std::string& name, const BT::NodeConfig& config)
    : BT::ConditionNode(name, config)
  {
    node_ = rclcpp::Node::make_shared("check_battery_node");
    sub_ = node_->create_subscription<std_msgs::msg::Bool>(
      "/is_battery_low", 10,
      [this](const std_msgs::msg::Bool::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        is_low_ = msg->data;
      });
  }

  BT::NodeStatus tick() override
  {
    rclcpp::spin_some(node_);
    std::lock_guard<std::mutex> lock(mutex_);
    RCLCPP_INFO(node_->get_logger(), "[BT] CheckBatteryLow: %s", is_low_ ? "LOW" : "OK");
    return is_low_ ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
  }

  static BT::PortsList providedPorts() { return {}; }

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr sub_;
  bool is_low_ = false;
  std::mutex mutex_;
};

// ============================================================
// УЗЕЛ 2: NavigateToCharger (StatefulActionNode)
// ============================================================
class NavigateToCharger : public BT::StatefulActionNode
{
public:
  NavigateToCharger(const std::string& name, const BT::NodeConfig& config)
    : BT::StatefulActionNode(name, config)
  {
    node_ = rclcpp::Node::make_shared("navigate_to_charger_node");
    client_ = rclcpp_action::create_client<NavigateToPose>(node_, "navigate_to_pose");
    goal_accepted_ = false;
    goal_completed_ = false;
    goal_success_ = false;
  }

  BT::NodeStatus onStart() override
  {
    RCLCPP_INFO(node_->get_logger(), "[BT] NavigateToCharger: START");
    
    if (!client_->wait_for_action_server(10s)) {
      RCLCPP_ERROR(node_->get_logger(), "Nav2 action server not available!");
      return BT::NodeStatus::FAILURE;
    }

    double x, y;
    if (!getInput("x", x) || !getInput("y", y)) {
      RCLCPP_ERROR(node_->get_logger(), "Missing x or y input ports");
      return BT::NodeStatus::FAILURE;
    }

    NavigateToPose::Goal goal;
    goal.pose.header.frame_id = "map";
    goal.pose.header.stamp = node_->get_clock()->now();
    goal.pose.pose.position.x = x;
    goal.pose.pose.position.y = y;
    goal.pose.pose.orientation.w = 1.0;

    RCLCPP_INFO(node_->get_logger(), "[BT] Sending goal to (%.2f, %.2f)", x, y);

    auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
    
    send_goal_options.goal_response_callback = 
      [this](rclcpp_action::ClientGoalHandle<NavigateToPose>::SharedPtr goal_handle) {
        if (!goal_handle) {
          RCLCPP_ERROR(node_->get_logger(), "Goal rejected!");
          goal_accepted_ = false;
        } else {
          RCLCPP_INFO(node_->get_logger(), "Goal accepted by Nav2");
          goal_accepted_ = true;
        }
      };

    send_goal_options.result_callback = 
      [this](const rclcpp_action::ClientGoalHandle<NavigateToPose>::WrappedResult& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (result.code == rclcpp_action::ResultCode::SUCCEEDED) {
          RCLCPP_INFO(node_->get_logger(), "[BT] Navigation SUCCEEDED");
          goal_success_ = true;
        } else {
          RCLCPP_ERROR(node_->get_logger(), "[BT] Navigation FAILED with code %d", static_cast<int>(result.code));
          goal_success_ = false;
        }
        goal_completed_ = true;
      };

    goal_accepted_ = false;
    goal_completed_ = false;
    goal_success_ = false;
    
    client_->async_send_goal(goal, send_goal_options);
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    rclcpp::spin_some(node_);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (goal_completed_) {
      return goal_success_ ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }
    
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override
  {
    RCLCPP_INFO(node_->get_logger(), "[BT] NavigateToCharger: HALTED - canceling goal");
    client_->async_cancel_all_goals();
  }

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<double>("x"),
      BT::InputPort<double>("y")
    };
  }

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp_action::Client<NavigateToPose>::SharedPtr client_;
  bool goal_accepted_;
  bool goal_completed_;
  bool goal_success_;
  std::mutex mutex_;
};

// ============================================================
// УЗЕЛ 3: WaitForCharge (ConditionNode)
// ============================================================
class WaitForCharge : public BT::ConditionNode
{
public:
  WaitForCharge(const std::string& name, const BT::NodeConfig& config)
    : BT::ConditionNode(name, config)
  {
    node_ = rclcpp::Node::make_shared("wait_for_charge_node");
    sub_ = node_->create_subscription<std_msgs::msg::Float32>(
      "/battery_level", 10,
      [this](const std_msgs::msg::Float32::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        current_level_ = msg->data;
      });
  }

  BT::NodeStatus tick() override
  {
    rclcpp::spin_some(node_);
    
    double target = 80.0;
    if (!getInput("target_level", target)) {
      target = 80.0;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    RCLCPP_INFO(node_->get_logger(), "[BT] WaitForCharge: %.1f%% / %.1f%%", current_level_, target);
    
    if (current_level_ >= target) {
      return BT::NodeStatus::SUCCESS;
    }
    
    // Небольшая пауза, чтобы не спамить логами
    std::this_thread::sleep_for(500ms);
    return BT::NodeStatus::RUNNING;
  }

  static BT::PortsList providedPorts()
  {
    return { BT::InputPort<double>("target_level") };
  }

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr sub_;
  float current_level_ = 0.0;
  std::mutex mutex_;
};

// ============================================================
// MAIN: Запуск BT executor
// ============================================================
int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto main_node = std::make_shared<rclcpp::Node>("bt_executor_main");
  
  RCLCPP_INFO(main_node->get_logger(), "=== BT Executor Starting ===");

  BT::BehaviorTreeFactory factory;
  factory.registerNodeType<CheckBatteryLow>("CheckBatteryLow");
  factory.registerNodeType<NavigateToCharger>("NavigateToCharger");
  factory.registerNodeType<WaitForCharge>("WaitForCharge");

  std::string bt_xml = "/home/pate/Practice_PP/ws_ros2/install/power_nav_bt_plugins/share/power_nav_bt_plugins/behavior_trees/charge_battery_tree.xml";
  
  BT::Tree tree;
  try {
    tree = factory.createTreeFromFile(bt_xml);
  } catch (const std::exception& e) {
    RCLCPP_ERROR(main_node->get_logger(), "Failed to load BT: %s", e.what());
    rclcpp::shutdown();
    return 1;
  }

  RCLCPP_INFO(main_node->get_logger(), "=== BT Loaded, starting tick loop ===");

  rclcpp::Rate rate(2);  // 2 тика в секунду
  while (rclcpp::ok()) {
    rclcpp::spin_some(main_node);
    BT::NodeStatus status = tree.tickWhileRunning();
    
    if (status == BT::NodeStatus::SUCCESS) {
      RCLCPP_INFO(main_node->get_logger(), "=== Tree SUCCESS (charging complete) ===");
      // После успешной зарядки можно перезапустить дерево
      tree.haltTree();
    } else if (status == BT::NodeStatus::FAILURE) {
      // Батарея в норме - дерево не выполняется (ReactiveSequence вернул FAILURE)
      // Просто ждём
    }
    
    rate.sleep();
  }

  rclcpp::shutdown();
  return 0;
}