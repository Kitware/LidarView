cmake_minimum_required(VERSION 3.20.3 FATAL_ERROR)
project(LidarView)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(SetBranding)
include(LidarViewSupportMacros)
include(LidarViewOptions)

include(SetupLidarviewProject)

# Add plugins to be searched by default in LVCore CMakeLists.txt
set(lidarview_available_interpreters_plugins
  VelodynePlugin
  LeishenPlugin
  LivoxPlugin
  RobosensePlugin
  HesaiPlugin)

if (LIDARVIEW_USE_CURL)
  list(APPEND lidarview_available_interpreters_plugins
    OusterPlugin
  )
endif()

set(lidarview_interpreters_plugin_directories "${CMAKE_CURRENT_SOURCE_DIR}/Plugins")
add_subdirectory(LVCore)

# Build lidarview application & QT module specific
add_subdirectory(Application)

if (BUILD_DOCUMENTATION)
  add_subdirectory(Documentation/Sphinx)
endif ()

if (BUILD_DEVELOPER_DOCUMENTATION)
  add_subdirectory(Utilities/Doxygen)
endif ()
