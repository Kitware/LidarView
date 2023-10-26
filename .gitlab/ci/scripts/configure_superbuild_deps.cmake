if (NOT DEFINED "${QTBINARIES}")
  message(FATAL_ERROR "QTBINARIES is not defined.")
else()
  get_filename_component(qt5_cmake_dir "${QTBINARIES}/../lib/cmake/Qt5" ABSOLUTE)
  message(STATUS "Found Qt5 at ${qt5_cmake_dir}")
endif()

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
set(BUILD_TESTING OFF CACHE STRING "")

# Disable lidarview as will be built later.
set(ENABLE_lidarview OFF CACHE STRING "")

# LidarView required dependencies
set(ENABLE_paraview ON CACHE STRING "")
set(ENABLE_qt5 ON CACHE STRING "")
set(ENABLE_pcap ON CACHE STRING "")
set(ENABLE_boost ON CACHE STRING "")
set(ENABLE_eigen ON CACHE STRING "")
set(ENABLE_las ON CACHE STRING "")
set(ENABLE_yaml ON CACHE STRING "")
set(ENABLE_python3 ON CACHE STRING "")
set(ENABLE_numpy ON CACHE STRING "")

# LidarView optionnal dependencies
set(ENABLE_slam ON CACHE STRING "")
set(ENABLE_hesaisdk ON CACHE STRING "")
set(ENABLE_pcl ON CACHE STRING "")
set(ENABLE_ceres ON CACHE STRING "")
set(ENABLE_opencv OFF CACHE STRING "")
set(ENABLE_nanoflann ON CACHE STRING "")

# System Qt5
set(USE_SYSTEM_qt5 ON CACHE STRING "")
set(Qt5_DIR ${qt5_cmake_dir} CACHE STRING "")
