cmake_minimum_required(VERSION 3.5)
project(publisher)

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
find_package(spdlog REQUIRED PATHS ~/install/spdlog-1.12.0)

add_executable(talker src/publisher.cpp)
ament_target_dependencies(talker rclcpp std_msgs)
target_link_libraries(talker spdlog::spdlog)

install(TARGETS
  talker
  DESTINATION lib/${PROJECT_NAME})

install(
  FILES ${PROJECT_BINARY_DIR}/compile_commands.json
  DESTINATION ${PROJECT_SOURCE_DIR}
  )

ament_package()
