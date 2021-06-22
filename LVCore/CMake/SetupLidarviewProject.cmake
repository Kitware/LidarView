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
if(NOT superbuild_python_version)
  message(FATAL_ERROR "superbuild_python_version not set")
endif()
if(NOT paraview_version)
  message(FATAL_ERROR "paraview_version not set")
endif()

include(CheckBuildType)

#-------------------------------------------------------------------------------
# Set Variables
option(BUILD_SHARED_LIBS "Build shared libs" ON) #Should be a Set instead of an Option
include(SetCompilationFlags)

# Branding
add_definitions( -DPROJECT_NAME="${SOFTWARE_NAME}" )
add_definitions( -DSOFTWARE_NAME="${SOFTWARE_NAME}" )
add_definitions( -DSOFTWARE_VENDOR="${SOFTWARE_VENDOR}" )

# Advertise Python version
add_definitions( -DLV_PYTHON_VERSION=${superbuild_python_version})

#-------------------------------------------------------------------------------
# Dependencies
include(Git)
include(CTest)

# Find Python
message("Using Python ${superbuild_python_version}")
find_package(Python3 ${superbuild_python_version} EXACT QUIET REQUIRED COMPONENTS Interpreter Development)
if(NOT ${superbuild_python_version} EQUAL "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
  message(FATAL_ERROR "Superbuild and found Python Version mismatch")
endif()

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
# WIP should check if EQUAL ${paraview_version}, in the event of from-source builds

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

#-------------------------------------------------------------------------------
# Set Relative Path Variables
include(SetupOutputDirs)

#-------------------------------------------------------------------------------
# Set Absolute Variables

# Set default OUTPUT_DIRECTORY, like PV prefer those instead of platform-specifics ones
# This is the most common for third-parties be GNUInstallDirs compliant,
# so a 'fixup-install-dir' step is required anyway
# Remember the common-cb/CMakeLists.txt sets CMAKE_INSTALL_PREFIX for install for every projects
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(UNIX OR APPLE)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
else()
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Setup Python module DIR
if(NOT Python3_VERSION_MAJOR OR NOT Python3_VERSION_MINOR)
  message(FATAL_ERROR "Python3_VERSION_ variables not set")
endif()
if(WIN32)
  set(LV_INSTALL_PYTHON_MODULES_DIR "${LV_INSTALL_LIBRARY_DIR}/Lib/site-packages")
elseif(APPLE)
  set(LV_INSTALL_PYTHON_MODULES_DIR "${LV_INSTALL_LIBRARY_DIR}/../Python") #wip use only Python ? or ${CMAKE_BINARY_DIR}  ${CMAKE_INSTALL_PREFIX} ?
else()
  set(LV_INSTALL_PYTHON_MODULES_DIR "${LV_INSTALL_LIBRARY_DIR}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages")
endif()

#-------------------------------------------------------------------------------
# RPATH Legacy notes
# This is not necessarry anymore thanks to the behavior of PV Client macros
# Setting this globally will pollute Plugins, because of PVPlugin.cmake 'saved_rpath' mechanisms
# Optionally set this on a local basis for libraries, and sometimes recommend as the package fixup-bundle script
# will assume that it relies only on system-libs and the binary will wrongly link on undesried system libs (See Project/PythonQt)
# SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# WIPWIP TODO APPLE make sure on apple its necessary or not
#[[
if (APPLE)
  set(CMAKE_INSTALL_NAME_DIR "@executable_path/../Libraries")
  #wip not even sure it does anything I dont see a single @executable_path
  #@rpath is more flexible #set CMAKE_MACOSX_RPATH
  #CMAKE_INSTALL_NAME_DIR

  # ensure that we don't build forwarding executables on apple.
  set(VTK_BUILD_FORWARDING_EXECUTABLES FALSE)
endif()
]]

#-------------------------------------------------------------------------------
# Modules
add_subdirectory(LVCore)

# Fixup-Install
# On windows, we install all needed tools manually in install dir
if (WIN32)
  # Ship Qt5, Python3
  include(SetupWindowsCustomInstall)
endif ()



