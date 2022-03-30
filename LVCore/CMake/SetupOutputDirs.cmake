#===============================================================================
# Copyright 2021 Kitware, Inc.
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
#===============================================================================

#------------------------------------------------------------------------------
# Set Install Architecture Relative Variables
# WARNING ALL THOSE SHOULD REMAIN RELATIVE FOR USE WITH SB-PACKAGING
# Relative from ${CMAKE_PREFIX_PATH}, either /install or package build dir

# Sanitize checks
if(NOT SOFTWARE_NAME )
  message(FATAL_ERROR "SOFTWARE_NAME branding not set")
endif()

# Setup install runtime directory
set(LV_INSTALL_RUNTIME_DIR bin)

# Setup install library directory
if (WIN32)
  set(LV_INSTALL_LIBRARY_DIR bin)
elseif(APPLE)
  set(LV_INSTALL_LIBRARY_DIR Applications/${SOFTWARE_NAME}.app/Contents/Libraries)
elseif(UNIX)
  set(LV_INSTALL_LIBRARY_DIR lib)
else()
  message(FATAL_ERROR "Platform unsupported")
endif()

# Setup build LV Plugin subdir # For use in PV Plugin build macros
# WARNING: RELATIVE TO BUILD DIR NOT INSTALL
set(LV_PLUGIN_BUILD_SUBDIRECTORY "plugins")

# Setup install LV Plugin subdir # Where it should be installed / packaged
set(LV_PLUGIN_INSTALL_SUBDIRECTORY "${LV_INSTALL_LIBRARY_DIR}/plugins")

# Setup install PV Plugin subdir
if (WIN32)
  set(LV_INSTALL_PV_PLUGIN_SUBDIR "${LV_INSTALL_LIBRARY_DIR}/paraview-${paraview_version}/plugins")
elseif (APPLE)
  set(LV_INSTALL_PV_PLUGIN_SUBDIR "lib/paraview-5.9/plugins") # This one is specific
elseif (UNIX)
  set(LV_INSTALL_PV_PLUGIN_SUBDIR "${LV_INSTALL_LIBRARY_DIR}/paraview-${paraview_version}/plugins")
endif ()

# Setup install Include dir
set(LV_INSTALL_INCLUDE_DIR "include") #wipwip ideally with version suffix like pv

# Setup install Ressources path
if(APPLE)
  set(LV_INSTALL_RESOURCE_DIR "Applications/${SOFTWARE_NAME}.app/Contents/Resources")
else()
  set(LV_INSTALL_RESOURCE_DIR "share")
endif()
