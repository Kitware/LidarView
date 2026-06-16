paraview_add_plugin(MotionDetectorToolbox
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES
    MotionDetectorToolbox::IO
  MODULE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/IO/vtk.module"
)
