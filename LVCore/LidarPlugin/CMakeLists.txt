cmake_minimum_required(VERSION 3.18)
project(LidarPlugin  C CXX)

set(BUILD_SHARED_LIBS ON)

paraview_plugin_scan(
  PLUGIN_FILES      "${CMAKE_CURRENT_LIST_DIR}/Plugin/paraview.plugin"
  ENABLE_BY_DEFAULT ON #You can use this instead of the variables above
  PROVIDES_PLUGINS  plugins
  REQUIRES_MODULES  required_modules)

foreach (module IN LISTS required_modules)
  if (NOT TARGET "${module}")
    message(FATAL_ERROR "LidarPlugin missing required module ${module}")
  endif ()
endforeach ()

paraview_plugin_build(
  HEADERS_DESTINATION   "${LV_INSTALL_INCLUDE_DIR}"
  RUNTIME_DESTINATION   "${LV_INSTALL_RUNTIME_DIR}"
  LIBRARY_DESTINATION   "${LV_INSTALL_LIBRARY_DIR}"
  LIBRARY_SUBDIRECTORY  "${LV_PLUGIN_BUILD_SUBDIRECTORY}"
  PLUGINS_FILE_NAME "lidarview.LidarPlugin.xml"
  PLUGINS ${plugins}
  AUTOLOAD LidarPlugin
)
