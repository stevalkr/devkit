cmake_minimum_required(VERSION 3.5)
project(subscriber)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(listener src/subscriber.cpp)
ament_target_dependencies(listener rclcpp std_msgs)

install(TARGETS
  listener
  DESTINATION lib/${PROJECT_NAME})

install(
  FILES ${PROJECT_BINARY_DIR}/compile_commands.json
  DESTINATION ${PROJECT_SOURCE_DIR}
  )

ament_package()
