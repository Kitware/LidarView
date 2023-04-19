set(classes
  lqHelper)

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)

vtk_module_add_module(LidarView::lqCore
  FORCE_STATIC
  CLASSES ${classes}
)

vtk_module_link(LidarView::lqCore
  PRIVATE
    Qt5::Core
    Qt5::Widgets
)
vtk_module_definitions(LidarView::lqCore
  PRIVATE
    QT_NO_KEYWORDS
)
