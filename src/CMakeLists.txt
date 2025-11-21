qt_add_executable(robot_visualizer
  TimelineWidget.cpp
  main.cpp
)

target_include_directories(robot_visualizer PRIVATE ../include/)
target_link_libraries(robot_visualizer PRIVATE Qt6::Core)
target_link_libraries(robot_visualizer PRIVATE Qt6::Widgets)
