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

#CXX Standard #Overrides SB setting it to 11
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Building LidarView with C++${CMAKE_CXX_STANDARD} standard by default")

# MSVC MT is not enforced # Wip to investigate
if(WIN32 AND MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# MSVC WINNT Version
if (WIN32 AND CMAKE_SYSTEM_VERSION)
    set(LV_MSVC_VER ${CMAKE_SYSTEM_VERSION})
    string(REPLACE "." "" LV_MSVC_VER ${LV_MSVC_VER})
    string(REGEX REPLACE "([0-9])" "0\\1" LV_MSVC_VER ${LV_MSVC_VER})
    set(LV_MSVC_VER 0x${LV_MSVC_VER})
    message(STATUS "Using _WIN32_WINNT=${LV_MSVC_VER}")
    add_definitions(-D_WIN32_WINNT=${LV_MSVC_VER})
endif()

#Old MSVC did not recognize "inline" but only "__inline"
#Some Thirdparties still try to do the forbidden '#define inline __inline'
if(WIN32 AND MSVC)
  if(MSVC_VERSION GREATER 1699)
    add_definitions(/D_ALLOW_KEYWORD_MACROS)
  endif()
endif()

# Warnings
if (MSVC)
  # Warning level 3, but disable some noisy warnings
  add_compile_options(/W3
                      /wd4244  # 'argument' : conversion from 'type1' to 'type2', possible loss of data
                      /wd4267  # 'var' : conversion from 'size_t' to 'type', possible loss of data
                      /wd4800  # 'type1': forcing value to bool 'true' or 'false'
                      /wd4141  # 'modifier' : used more than once
                      /wd4251  # 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'
                      /wd4503  # 'identifier' : decorated name length exceeded, name was truncated
  )
elseif(APPLE)
  add_compile_options(-Wall -Wextra)
elseif(UNIX)
  add_compile_options(-Wall -Wextra -Wsuggest-override)
else()
  message(FATAL_ERROR "Platform not supported")
endif()

# Optimize Build in Release
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
if (CMAKE_BUILD_TYPE_LOWER STREQUAL "release")
  # Optimization-Level
  if(MSVC)
    add_compile_options(/O2)
  else()
    add_compile_options(-O2)
  endif()

  #IPO
  include(CheckIPOSupported)
  check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
  if(IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(WARNING "IPO is not supported: ${IPO_ERROR}")
  endif()
endif()
