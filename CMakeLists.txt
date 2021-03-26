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

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(LidarView)
include("Application/SoftwareInformation/branding.cmake")

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_EXTENSIONS OFF)

if(WIN32 AND MSVC)
  if(MSVC_VERSION GREATER 1699)
    add_definitions(/D_ALLOW_KEYWORD_MACROS)
  endif()
  add_definitions(/Dinline=__inline)
endif()

# Is this a 32 bit or 64bit build. Display this in about dialog.
if ("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
  set(LV_BUILD_ARCHITECTURE "64")
else()
  set(LV_BUILD_ARCHITECTURE "32")
endif()

option(BUILD_DOC "Build documentation" OFF)
option(BUILD_SHARED_LIBS "Build shared libs" ON)

#-------------------------------------------------------------------------------
set (LidarView_CMAKE_DIR "${LidarView_SOURCE_DIR}/LVCore/CMake")
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LidarView_CMAKE_DIR})

include(SetCompilationWarnings)
include(CheckBuildType)

if(BUILD_DOC)
  include(SetupDoxygenDocumentation)
endif()

include(Git)
include(ParaViewDetermineVersion)
# Sets LV_VERSION_{MAJOR,MINOR,PATCH} using PARAVIEW determine_version
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "LV")
determine_version(${LidarView_SOURCE_DIR} ${GIT_EXECUTABLE} "LV")

include(CTest)

find_package(ParaView REQUIRED)

# Include findpythonlibs to get the function PYTHON_ADD_MODULE
# needed in WRAP_PLUGIN_FOR_PYTHON
# Warning : Do not use find_package(PythonLibs) because it reset 
# some PYTHON paths that can enter in conflicts with those found
# during the superbuild (specially right on APPLE when a second
# python version has been installed using homebrew)
include(FindPythonLibs)
# We force python to version 3.7 as it is the only one that has been tested
find_package(Python3 3.7 EXACT QUIET REQUIRED COMPONENTS Interpreter)

include(${PARAVIEW_USE_FILE})
include(SetupOutputDirs)

# Here we use a custom cmake file to find PythonQt and a PythonQtPlugin
find_package(PythonQt REQUIRED)

add_subdirectory(Plugins)
add_subdirectory(LVCore)
add_subdirectory(Application)

# On windows, we install all needed tools manually in install dir
if (WIN32)
  include(SetupWindowsCustomInstall)
endif ()

# reset boost RPATH on OSX
# this is applied on LidarView app and tests
# for more information see comments in lidarview.osx-boost-rpath.cmake
install(SCRIPT
  ${LidarView_SOURCE_DIR}/Superbuild/lidarview-superbuild/Projects/scripts/lidarview.osx-boost-rpath.cmake
)

