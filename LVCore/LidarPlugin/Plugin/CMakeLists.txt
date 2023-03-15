#--------------------------------------
# Scan and find all modules
#--------------------------------------
set(lidarplugin_source_directories "${CMAKE_CURRENT_SOURCE_DIR}/LidarCore")

vtk_module_find_modules(lidarplugin_module_files ${lidarplugin_source_directories})
vtk_module_scan(
  MODULE_FILES ${lidarplugin_module_files}
  PROVIDES_MODULES lidarplugin_modules
  WANT_BY_DEFAULT ON)

#--------------------------------------
# Create plugin
#--------------------------------------
paraview_add_plugin(LidarPlugin
  VERSION "2.0"
  MODULES ${lidarplugin_modules}
  MODULE_FILES ${lidarplugin_module_files}
  MODULE_ARGS
    LICENSE_DESTINATION "${LV_INSTALL_INCLUDE_DIR}" #wip Temporary fix
    TEST_DIRECTORY_NAME "None" # Deactivate testing - incompatible with LidarPlugin
    TEST_INPUT_DATA_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data"
)

#-----------------------------------------------------------------------------
# Python wrapping
# Simply use "import LidarPlugin.LidarCore" and you have acces to the wrapped c++ code
# DO NOT FOR ANY REASON USE the method GetClientSideObject() to have acces to the vtkObject directly.
#-----------------------------------------------------------------------------

if(NOT LV_INSTALL_PYTHON_MODULES_DIR)
  message(FATAL_ERROR "LV_INSTALL_PYTHON_MODULES_DIR not set")
endif()

set(lidarplugin_wrapped_modules
  LidarView::CommonCore
  LidarView::IOCore
  LidarView::IOGeneral
  LidarView::IOGeolocation
  LidarView::IONetwork
  LidarView::IOLidar
  LidarView::Sources
)

vtk_module_wrap_python(
  MODULES         ${lidarplugin_wrapped_modules}
  BUILD_STATIC    OFF
  INSTALL_HEADERS OFF
  PYTHON_PACKAGE  "lidarview.modules" # This is the name of the python module to import
  MODULE_DESTINATION  "${LV_INSTALL_PYTHON_MODULES_DIR}" #"." #does not really like #without CMAKE_INSTALL_PREFIX
  LIBRARY_DESTINATION "${LV_INSTALL_LIBRARY_DIR}"
  DEPENDS         VTK::vtkpythonmodules
)
