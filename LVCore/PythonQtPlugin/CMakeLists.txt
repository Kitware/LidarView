# Building the STATIC PythonQtPlugin ourselves 
# makes it possible to embbed it seamlessly in LidarView
cmake_minimum_required(VERSION 3.18)
project(PythonQtPlugin)

# Sanitize Checks
if(NOT PYTHONQTPLUGIN_DIR)
  message(FATAL_ERROR "PYTHONQTPLUGIN_DIR not FOUND")
endif()

#Request Static for embedding
set(BUILD_SHARED_LIBS OFF)

#set(_paraview_plugin_default_PythonQtPlugin "ON")

paraview_plugin_scan(
  PLUGIN_FILES      "${PYTHONQTPLUGIN_DIR}/paraview.plugin"
  ENABLE_BY_DEFAULT ON #You can use this instead of the variables above
  PROVIDES_PLUGINS  plugins
  REQUIRES_MODULES  required_modules)

foreach (module IN LISTS required_modules)
  if (NOT TARGET "${module}")
    message(FATAL_ERROR "PythonQtPlugin missing required module ${module}")
  endif ()
endforeach ()

paraview_plugin_build(
  HEADERS_DESTINATION   "${LV_INSTALL_INCLUDE_DIR}"
  RUNTIME_DESTINATION   "${LV_INSTALL_RUNTIME_DIR}"
  LIBRARY_DESTINATION   "${LV_INSTALL_LIBRARY_DIR}"
  LIBRARY_SUBDIRECTORY  "${LV_PLUGIN_BUILD_SUBDIRECTORY}"
  PLUGINS_FILE_NAME "lidarview.PythonQtPlugin.xml"
  PLUGINS ${plugins}
  AUTOLOAD PythonQtPlugin
)

