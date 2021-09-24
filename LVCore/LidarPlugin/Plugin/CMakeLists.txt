# Create Plugin
paraview_add_plugin(LidarPlugin 
  VERSION "1.0"
  MODULES LidarCore
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/LidarCore/vtk.module"
  MODULE_ARGS
    LICENSE_DESTINATION "${LV_INSTALL_INCLUDE_DIR}" #wip Temporary fix
    TEST_DIRECTORY_NAME "NONE" # Do not let vtk_module build the default 'Testing' subdir, it complexifies paths
)
