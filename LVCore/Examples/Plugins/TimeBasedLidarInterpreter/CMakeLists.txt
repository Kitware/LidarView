# Create the paraview plugin with the vtk module path
paraview_add_plugin(TimeBasedLidarInterpreter
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "2.0"
  MODULES
    TimeBasedLidarInterpreter::TimeBasedLidarPacketInterpreter
  MODULE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/TimeBasedLidarPacketInterpreter/vtk.module"
  SERVER_MANAGER_XML
    "${CMAKE_CURRENT_SOURCE_DIR}/TimeBasedLidarExampleProxies.xml"
)
