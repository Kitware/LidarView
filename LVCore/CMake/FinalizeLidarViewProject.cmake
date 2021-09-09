# Sanitize checks
if(NOT SOFTWARE_NAME)
  message(FATAL_ERROR "SOFTWARE_NAME branding not set")
endif()
if(NOT LV_INSTALL_LIBRARY_DIR)
  message(FATAL_ERROR "LV_INSTALL_LIBRARY_DIR not set")
endif()
if(NOT LV_PLUGIN_BUILD_SUBDIRECTORY)
  message(FATAL_ERROR "LV_PLUGIN_BUILD_SUBDIRECTORY not set")
endif()
if(NOT LV_PLUGIN_INSTALL_SUBDIRECTORY)
  message(FATAL_ERROR "LV_PLUGIN_INSTALL_SUBDIRECTORY not set")
endif()
if(NOT paraview_version )
  message(FATAL_ERROR "paraview_version not set")
endif()

# WIPWIP DISABLED FOR NOW, MOST LIKELY NOT NEEDED ON PV59
#[[
# Patch reset boost/flann RPATH on OSX
# this is applied on LidarView app and tests
# for more information see comments in lidarview.osx-rpath.cmake
install(SCRIPT "LVCore/CMake/scripts/lidarview.osx-rpath.cmake")
]]

# Generate Configuration File (Plugins AutoLoad) #wipwip will mostlikely need to fecth them from builddir/LV_PLUGIN_INSTALL_SUBRID
# Example : 'install/bin/LidarView.conf' must contain '../lib/plugins/lidarview.plugins.xml'
set(genconf_script "GenerateConfiguration.cmake")
configure_file("LVCore/CMake/${genconf_script}.in" "${CMAKE_CURRENT_BINARY_DIR}/${genconf_script}" @ONLY)
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/${genconf_script})

# Output CMakeCacheLog.txt
install(SCRIPT "LVCore/CMake/OutputCMakeCacheLog.cmake")
