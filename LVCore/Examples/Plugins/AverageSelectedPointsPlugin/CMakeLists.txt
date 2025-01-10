set(interfaces)
set(sources
  lq2DBoxPropertyWidget.cxx
  lq2DBoxPropertyWidget.h)

paraview_plugin_add_property_widget(
  KIND GROUP_WIDGET
  TYPE "MyInteractive2DBox"
  CLASS_NAME lq2DBoxPropertyWidget
  INTERFACES property_interfaces
  SOURCES property_sources)
list(APPEND interfaces
  ${property_interfaces})
list(APPEND sources
  ${property_sources})

paraview_add_plugin(AverageSelectedPointsPlugin
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  MODULES AverageSelectedPoints
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/AverageSelectedPoints/vtk.module"
  SERVER_MANAGER_XML AverageSelectedPoints.xml
)

set(required_modules
  ParaView::pqApplicationComponents
  ParaView::RemotingViews
)
target_link_libraries(AverageSelectedPointsPlugin PRIVATE ${required_modules})
