set(classes
  lqHelper
  lqStreamRecorderInterface
)

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)

vtk_module_add_module(LidarView::lqCore
  FORCE_STATIC
  CLASSES ${classes}
  HEADERS ${headers}
)

vtk_module_link(LidarView::lqCore
  PRIVATE
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core"
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets"
)
vtk_module_definitions(LidarView::lqCore
  PRIVATE
    QT_NO_KEYWORDS
)
