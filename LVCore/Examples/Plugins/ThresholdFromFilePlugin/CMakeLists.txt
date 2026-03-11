paraview_add_plugin(ThresholdFromFilePlugin
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES ThresholdFromFile
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ThresholdFromFile/vtk.module")
