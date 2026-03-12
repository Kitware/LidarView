# Copyright 2013 Velodyne Acoustics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#CMake
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
include(Git)
include(CTest)

# Version
include(ParaViewDetermineVersion)
# Sets LIDARVIEW_VERSION_{MAJOR,MINOR,PATCH} using PARAVIEW determine_version
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "LIDARVIEW")
determine_version(${CMAKE_SOURCE_DIR} ${GIT_EXECUTABLE} "LIDARVIEW")

include(FindLidarViewDependencies)

if (BUILD_DEVELOPER_DOCUMENTATION)
  include(SetupDoxygenDocumentation)
endif ()

if (LIDARVIEW_USE_PYTHON)
  vtk_module_python_default_destination(LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX)
endif ()
