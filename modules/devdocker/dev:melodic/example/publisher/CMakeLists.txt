cmake_minimum_required(VERSION 2.8.3)
project(publisher)

## Find catkin and any catkin packages
find_package(catkin REQUIRED COMPONENTS roscpp rospy std_msgs genmsg)
find_package(spdlog REQUIRED PATHS ~/install/spdlog-1.12.0)

## Declare a catkin package
catkin_package()

## Build talker and listener
include_directories(include ${catkin_INCLUDE_DIRS})

add_executable(talker src/publisher.cpp)
target_link_libraries(talker ${catkin_LIBRARIES} spdlog::spdlog)

