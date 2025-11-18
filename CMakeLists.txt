cmake_minimum_required(VERSION 3.16)
project(robot_visualizer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(AUTOMOC ON)

# find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()


set(SOURCES main.cpp)

qt_add_executable(robot_visualizer
	${SOURCES}
)

target_include_directories(robot_visualizer PRIVATE include/)

# target_link_libraries(robot_visualizer PRIVATE Qt6::Core)
target_link_libraries(robot_visualizer PRIVATE Qt6::Widgets)
