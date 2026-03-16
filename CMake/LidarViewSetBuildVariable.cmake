include(GNUInstallDirs)
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
endif ()
if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
endif ()
if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
endif ()

# Disallow in-source build
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
  message(FATAL_ERROR
    "LidarView requires an out-of-source build. Please create a separate "
    "binary directory and run CMake command there giving "
    "in parameter the LidarView source directory.")
endif ()

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE
    PROPERTY
      STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set Variables
include(SetCompilationFlags)

# Sets LIDARVIEW_VERSION_{MAJOR,MINOR,PATCH} using PARAVIEW determine_version
include(ParaViewDetermineVersion)
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "LIDARVIEW")
determine_version(${CMAKE_SOURCE_DIR} ${GIT_EXECUTABLE} "LIDARVIEW")

if (LIDARVIEW_USE_PYTHON)
  # Set LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX variable
  vtk_module_python_default_destination(LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX)
endif ()

if (BUILD_TESTING)
  include(CTest)
endif ()
