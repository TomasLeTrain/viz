qt_add_executable(robot_visualizer
  TimelineWidget.cpp
  FieldView.cpp
  Point.cpp
  DraggableEllipseItem.cpp
  ComponentCard.cpp

  main.cpp
)


target_include_directories(robot_visualizer PRIVATE ../include/)

target_link_libraries(robot_visualizer PRIVATE Qt6::Core)
target_link_libraries(robot_visualizer PRIVATE Qt6::Widgets)
