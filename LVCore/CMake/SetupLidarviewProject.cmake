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
cmake_policy(SET CMP0057 NEW) #if() supports IN_LIST #ParaviewPlugin.cmake

#This allows incremental build within the ExternalProject dir `lidarview-sb/common-sb/lidarview/build`
set(SOFTWARE_NAME   ${SOFTWARE_NAME}   CACHE INTERNAL "LidarView-Based app name")
set(SOFTWARE_VENDOR ${SOFTWARE_VENDOR} CACHE INTERNAL "LidarView-Based app vendor")
set(LV_BUILD_ARCHITECTURE ${LV_BUILD_ARCHITECTURE} CACHE INTERNAL "Architecture address byte width")

#Sanitize checks
if(NOT SOFTWARE_NAME OR NOT SOFTWARE_VENDOR)
  message(FATAL_ERROR "SOFTWARE_NAME or SOFTWARE_VENDOR branding not set")
endif()

#Set Variables
option(BUILD_SHARED_LIBS "Build shared libs" ON) #Should be a Set instead of an Option
include(SetCompilationFlags)

#Branding
add_definitions( -DPROJECT_NAME="${SOFTWARE_NAME}" )
add_definitions( -DSOFTWARE_NAME="${SOFTWARE_NAME}" )
#add_definitions( -DSOFTWARE_VENDOR="${SOFTWARE_VENDOR}" ) # Not used

#Sanitize checks
include(CheckBuildType)

# Advertise Python version
add_definitions( -DLV_PYTHON_VERSION=${superbuild_python_version})

# Dependencies
include(Git)
include(CTest)

# Include findpythonlibs to get the function PYTHON_ADD_MODULE
# needed in WRAP_PLUGIN_FOR_PYTHON
# Warning : Do not use find_package(PythonLibs) because it reset 
# some PYTHON paths that can enter in conflicts with those found
# during the superbuild (specially right on APPLE when a second
# python version has been installed using homebrew)
include(FindPythonLibs)
# We force python to version 3.7 as it is the only one that has been tested
find_package(Python3 3.7 EXACT QUIET REQUIRED COMPONENTS Interpreter)

# Version
include(ParaViewDetermineVersion)
# Sets LV_VERSION_{MAJOR,MINOR,PATCH} using PARAVIEW determine_version
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "LV")
determine_version(${CMAKE_SOURCE_DIR} ${GIT_EXECUTABLE} "LV")

# Paraview
#must be included after Python and Determine version
if(NOT ParaView_DIR)
  message(WARNING "ParaView_DIR not found")
  message(FATAL_ERROR "Building with external Paraview not yet implemented")
endif()

find_package(ParaView REQUIRED)
message(STATUS "Paraview-${ParaView_VERSION}")

# VTK from Paraview
if(NOT VTK_DIR)
  message(WARNING "Is VTK provided by Paraview ?")
endif()
if(NOT VTK_FOUND )
  message(FATAL_ERROR "VTK not found")
endif()
if(NOT VTK_VERSION )
  message(FATAL_ERROR "VTK_VERSION not defined or empty, is VTK FOUND ?")
endif()
message(STATUS "VTK-${VTK_VERSION}")

# Qt5
if(NOT PARAVIEW_USE_QT)
  message(FATAL_ERROR "PARAVIEW_BUILD_QT_GUI is OFF, Paraview must be built with Qt")
endif()
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui UiTools Svg)
if(NOT Qt5_FOUND)
    message(FATAL_ERROR "Qt5 not found")
endif()
message(STATUS "Qt: ${PARAVIEW_QT_VERSION}, actually ${Qt5Core_VERSION}")

# Find PythonQt from Paraview's PythonQtPlugin Directory
# We also need to set this variable for PythonQtPlugin to build itself
# wip bad af, needs more checking
set(PYTHONQTPLUGIN_DIR "${CMAKE_INSTALL_PREFIX}/../lidarview-superbuild/common-superbuild/paraview/src/Plugins/PythonQtPlugin")
list(INSERT CMAKE_MODULE_PATH 0  "${PYTHONQTPLUGIN_DIR}/cmake")
find_package(PythonQt REQUIRED)
if(NOT PythonQt_FOUND OR NOT TARGET PythonQt::PythonQt)
  message(FATAL_ERROR "PythonQt::PythonQt not FOUND")
endif()

# Doc
option(BUILD_DOC "Build documentation" OFF)
if(BUILD_DOC)
  include(SetupDoxygenDocumentation)
endif()

# Setup Dir Variables
# - LV_INSTALL_LIBRARY_DIR
# - LV_INSTALL_PLUGIN_DIR
# - LV_INSTALL_PV_PLUGIN_DIR
# - LV_INSTALL_PYTHON_MODULES_DIR
# - Branding related variables already set
include(SetupOutputDirs)

# Modules
add_subdirectory(LVCore)

# On windows, we install all needed tools manually in install dir
if (WIN32)
  include(SetupWindowsCustomInstall)
endif ()



