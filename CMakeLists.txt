cmake_minimum_required(VERSION 3.20.3 FATAL_ERROR)
project(LidarView)

# Add path to get all the needed modules used to config Lidarview
set(lidarview_cmake_dir "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set(lvcore_cmake_dir "${CMAKE_CURRENT_SOURCE_DIR}/LVCore/CMake")
if (NOT IS_DIRECTORY "${lvcore_cmake_dir}")
  message(FATAL_ERROR
    "Failed to find the LVCore CMake directory. Did you forget to initialize the "
    "submodule? (git submodule update --init --recursive)")
endif ()
list(APPEND CMAKE_MODULE_PATH
  "${lidarview_cmake_dir}"
  "${lvcore_cmake_dir}")

include(SetBranding)
include(LidarViewSupportMacros)
include(LidarViewOptions)

include(SetupLidarviewProject)

# Add plugins to be searched by default in LVCore CMakeLists.txt
set(lidarview_available_interpreters_plugins
  VelodynePlugin
  LivoxPlugin
  HesaiPlugin)
set(lidarview_interpreters_plugin_directories "${CMAKE_CURRENT_SOURCE_DIR}/Plugins")
add_subdirectory(LVCore)

# Build lidarview application & QT module specific
add_subdirectory(Application)

if (BUILD_DOCUMENTATION)
  add_subdirectory(Documentation)
endif ()
