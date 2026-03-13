cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

include(CheckBuildType)

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

#-------------------------------------------------------------------------------
# Set Variables
include(SetCompilationFlags)

#-------------------------------------------------------------------------------
# Dependencies
include(CTest)
find_package(Git)

# Version
include(ParaViewDetermineVersion)
# Sets LIDARVIEW_VERSION_{MAJOR,MINOR,PATCH} using PARAVIEW determine_version
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "LIDARVIEW")
determine_version(${CMAKE_SOURCE_DIR} ${GIT_EXECUTABLE} "LIDARVIEW")

include(FindLidarViewDependencies)

if (LIDARVIEW_USE_PYTHON)
  vtk_module_python_default_destination(LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX)
endif ()
