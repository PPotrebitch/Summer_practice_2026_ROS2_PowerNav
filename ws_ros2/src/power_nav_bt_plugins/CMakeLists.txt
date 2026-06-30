cmake_minimum_required(VERSION 3.8)
project(power_nav_bt_plugins)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(behaviortree_cpp REQUIRED)

# Главный исполняемый файл
add_executable(bt_executor src/bt_executor.cpp)
ament_target_dependencies(bt_executor
  rclcpp
  rclcpp_action
  std_msgs
  geometry_msgs
  nav2_msgs
  behaviortree_cpp
)

# Устанавливаем исполняемый файл
install(TARGETS bt_executor
  DESTINATION lib/${PROJECT_NAME}
)

# Устанавливаем XML файлы деревьев
install(DIRECTORY behavior_trees/
  DESTINATION share/${PROJECT_NAME}/behavior_trees
)

ament_package()