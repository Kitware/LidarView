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

# Here we use a custom cmake file to find PythonQt and a PythonQtPlugin
find_package(PythonQt REQUIRED)

#PARAVIEW_USE_FILE must be included after Python
find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})

# Version
include(ParaViewDetermineVersion)
# Sets LV_VERSION_{MAJOR,MINOR,PATCH} using PARAVIEW determine_version
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "LV")
determine_version(${CMAKE_SOURCE_DIR} ${GIT_EXECUTABLE} "LV")

# Doc
option(BUILD_DOC "Build documentation" OFF)
if(BUILD_DOC)
  include(SetupDoxygenDocumentation)
endif()

# Setup Dir Variables
# - LV_INSTALL_LIBRARY_DIR
# - LV_INSTALL_PYTHON_MODULES_DIR
# - Branding related variables already set
include(SetupOutputDirs)


# Modules
add_subdirectory(Plugins)
add_subdirectory(LVCore)
add_subdirectory(Application)

# On windows, we install all needed tools manually in install dir
if (WIN32)
  include(SetupWindowsCustomInstall)
endif ()



