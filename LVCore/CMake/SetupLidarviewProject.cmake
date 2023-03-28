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

# Version
include(ParaViewDetermineVersion)
# Sets LV_VERSION_{MAJOR,MINOR,PATCH} using PARAVIEW determine_version
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "LV")
determine_version(${CMAKE_SOURCE_DIR} ${GIT_EXECUTABLE} "LV")

include(FindLidarViewDependencies)

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

vtk_module_python_default_destination(LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX)

#-----------------------------------------------------------------------------
# RPATH HANDLING
#-----------------------------------------------------------------------------
# ParaviewPLugin.cmake set CMAKE_INSTALL_RPATH RAPH to relpath between LIBRARY_DIR and plugin's LIBRARY_SUBDIR
# This will not for the execs, not inside /lib/plugins`
# We need to Set RPATHS ourselves here ParaViewClient.cmake Style

# RPATH Legacy notes
# This is not necessarry anymore thanks to the behavior of PV Client macros
# Setting this globally will pollute Plugins, because of PVPlugin.cmake 'saved_rpath' mechanisms
# Optionally set this on a local basis for libraries, and sometimes recommend as the package fixup-bundle script
# will assume that it relies only on system-libs and the binary will wrongly link on undesried system libs (See Project/PythonQt)
# SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Very Important for packaging to be able-to reverse associate Libraries at the end
if (APPLE)
  # Set BLANK RPATHS, make sure no @rpath in -rpath remain in libs
  #set(CMAKE_INSTALL_NAME_DIR "")
  set(CMAKE_MACOSX_RPATH TRUE)
  set(CMAKE_INSTALL_NAME_DIR "@executable_path/../Libraries")
  #set(CMAKE_INSTALL_NAME_DIR "@loader_path/../Libraries") #setting this will replace @rpath which is bad ??
  #@rpath does not work, but it says it is  "more flexible

elseif(UNIX)
  #WIP this is not necessarry
  #Set Explicit /lib DIR RPATH to avoid diamond depencencies issues (e.g libz)
  #list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN/../${LV_INSTALL_LIBRARY_DIR}")
endif()


#-------------------------------------------------------------------------------
# Modules
add_subdirectory(LVCore)

# Fixup-Install
# On windows, we install all needed tools manually in install dir
if (WIN32)
  # Ship Qt5, Python3
  include(SetupWindowsCustomInstall)
endif ()



