set(classes
  # ctk/ctkDoubleRangeSlider
  # ctk/ctkRangeSlider
  # ctk/ctkValueProxy
  lqHelper)

# set(headers
#   ctk/ctkCoreExport.h
#   ctk/ctkPimpl.h
#   ctk/ctkWidgetsExport.h)

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)

vtk_module_add_module(LidarView::lqCore
  FORCE_STATIC
  CLASSES ${classes}
  HEADERS ${headers})
# vtk_module_definitions(LidarView::lqCore
#   PRIVATE CTKCore_EXPORTS CTKWidgets_EXPORTS)
vtk_module_link(LidarView::lqCore
  PRIVATE
    Qt5::Core
    Qt5::Widgets
)
vtk_module_definitions(LidarView::lqCore
  PRIVATE
    QT_NO_KEYWORDS
)
