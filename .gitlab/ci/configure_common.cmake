# Stock CI builds test everything possible (platforms will disable modules as
# needed).
set(LIDARVIEW_BUILD_ALL_MODULES ON CACHE BOOL "")
set(BUILD_TESTING ON CACHE STRING "")

set(LIDARVIEW_USE_PCL ON CACHE STRING "")
set(LIDARVIEW_USE_CERES ON CACHE STRING "")
set(LIDARVIEW_USE_NANOFLANN ON CACHE STRING "")
set(LIDARVIEW_USE_OPENCV OFF CACHE STRING "")

# Default to Release builds.
if ("$ENV{CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
else ()
  set(CMAKE_BUILD_TYPE "$ENV{CMAKE_BUILD_TYPE}" CACHE STRING "")
endif ()

# Set up CMAKE_PREFIX_PATH for LVSB-DEPS for windows
if (WIN32)
  set(lvsb_dir)
  if (NOT DEFINED "ENV{LVSB_PATH}")
    get_filename_component(lvsb_dir "${CMAKE_CURRENT_LIST_DIR}/../../LVSB-DEPS/install/" ABSOLUTE)
  else ()
    get_filename_component(lvsb_dir "$ENV{LVSB_PATH}/install/" ABSOLUTE)
  endif ()
  list(APPEND lvsb_dir "${lvsb_dir}/Python")
  message(STATUS "CMAKE_PREFIX_PATH setted to ${lvsb_dir}")
  set(CMAKE_PREFIX_PATH "${lvsb_dir}" CACHE PATH "")

  set(Boost_USE_STATIC_LIBS OFF CACHE STRING "")
endif ()
