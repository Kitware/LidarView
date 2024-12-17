paraview_add_plugin(AverageSelectedPointsPlugin
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES AverageSelectedPoints
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/AverageSelectedPoints/vtk.module")
