paraview_add_plugin(DatasetIOPlugin
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES DatasetIOPlugin::vtkKittiIO
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/KittiIO/vtk.module"
)
