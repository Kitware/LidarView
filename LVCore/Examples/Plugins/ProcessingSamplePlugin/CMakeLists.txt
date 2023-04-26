paraview_add_plugin(ProcessingSamplePlugin
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES ProcessingSample
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ProcessingSample/vtk.module")
