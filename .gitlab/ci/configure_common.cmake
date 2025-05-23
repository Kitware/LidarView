# Stock CI builds test everything possible (platforms will disable modules as
# needed).
set(BUILD_ALL_MODULES ON CACHE BOOL "")
set(BUILD_EXAMPLES ON CACHE BOOL "")
set(BUILD_TESTING ON CACHE STRING "")

set(LIDARVIEW_USE_PDAL ON CACHE STRING "")
set(LIDARVIEW_USE_CERES ON CACHE STRING "")
set(LIDARVIEW_USE_NANOFLANN ON CACHE STRING "")
set(LIDARVIEW_USE_YAMLCPP ON CACHE STRING "")
set(LIDARVIEW_USE_OPENCV OFF CACHE STRING "")

# Default to Release builds.
if ("$ENV{CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
else ()
  set(CMAKE_BUILD_TYPE "$ENV{CMAKE_BUILD_TYPE}" CACHE STRING "")
endif ()
