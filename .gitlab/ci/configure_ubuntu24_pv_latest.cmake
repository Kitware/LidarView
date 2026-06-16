include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
set(CMAKE_PREFIX_PATH "/opt/paraview/install/" CACHE STRING "")

# They are too slow to build
set(LIDARVIEW_USE_CERES OFF CACHE STRING "" FORCE)
set(LIDARVIEW_USE_PDAL OFF CACHE STRING "" FORCE)
