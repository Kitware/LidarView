#--------------------------------------
# Miscellaneous helper cmake macros
#--------------------------------------
function(print_version name)
  message(STATUS "Found ${name}: ${${name}_VERSION}")
endfunction()

function(check_depedency_target name target_name)
  if (NOT TARGET ${target_name})
    message(FATAL_ERROR "Missing ${name} dependency.")
  endif ()
endfunction()

#--------------------------------------
# Python dependency - required
#--------------------------------------
find_package(Python3 3.8 QUIET REQUIRED COMPONENTS Interpreter)
set(lidarview_python_version "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
message(STATUS "Using Python ${lidarview_python_version}")

#--------------------------------------
# ParaView dependency - required
#--------------------------------------
find_package(ParaView 5.11 REQUIRED)
set(paraview_version "${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}")
if (NOT PARAVIEW_USE_QT)
  message(FATAL_ERROR "PARAVIEW_USE_QT is OFF, Paraview must be built with Qt")
endif ()
if (NOT PARAVIEW_USE_PYTHON)
  message(FATAL_ERROR "PARAVIEW_USE_PYTHON is OFF, Paraview must be built with Python")
endif ()
print_version(ParaView)

#--------------------------------------
# VTK (from ParaView package) dependency - required
#--------------------------------------
if (NOT VTK_FOUND OR NOT VTK_VERSION)
  message(FATAL_ERROR "VTK not found")
endif ()
if (NOT VTK_QT_VERSION VERSION_EQUAL "5")
  message(FATAL_ERROR "Qt5 was not used to build VTK")
endif ()
print_version(VTK)

#--------------------------------------
# Qt5 dependency - required
#--------------------------------------
find_package(Qt5 REQUIRED COMPONENTS Core Gui Help PrintSupport UiTools Svg Widgets)
if (NOT Qt5_FOUND)
  message(FATAL_ERROR "Qt5 not found")
endif ()
print_version(Qt5)

#--------------------------------------
# PythonQt dependency - required
#--------------------------------------
if (LIDARVIEW_USE_PYTHONQT)
  find_package(PythonQt REQUIRED)
  check_depedency_target(PythonQt PythonQt::PythonQt)
  message(STATUS "Found PythonQt")
endif ()

#--------------------------------------
# PCAP dependency - required
#--------------------------------------
if (LIDARVIEW_USE_PCAP)
  if (WIN32)
    find_library(PCAP_LIBRARY wpcap DOC "pcap library")
  else ()
    find_library(PCAP_LIBRARY pcap DOC "pcap library")
  endif ()
  find_path(PCAP_INCLUDE_DIR pcap.h DOC "pcap include directory")
  if (PCAP_LIBRARY AND PCAP_INCLUDE_DIR)
    add_library(PCAP::pcap UNKNOWN IMPORTED)
    set_target_properties(PCAP::pcap
      PROPERTIES
        IMPORTED_LOCATION "${PCAP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PCAP_INCLUDE_DIR}")
  endif ()
  check_depedency_target(pcap PCAP::pcap)
  message(STATUS "Found PCAP library")
endif ()

#--------------------------------------
# Liblas dependency - required
#--------------------------------------
if (LIDARVIEW_USE_LIBLAS)
  set(las_name las)
  if (WIN32)
    set(las_name liblas)
  endif ()

  find_library(LIBLAS_LIBRARY ${las_name} DOC "las library")
  find_path(LIBLAS_INCLUDE_DIR liblas/version.hpp DOC "las include directory")
  if (LIBLAS_LIBRARY AND LIBLAS_INCLUDE_DIR)
    add_library(LIBLAS::las UNKNOWN IMPORTED)
    set_target_properties(LIBLAS::las
      PROPERTIES
        IMPORTED_LOCATION "${LIBLAS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBLAS_INCLUDE_DIR}")
  endif ()
  check_depedency_target(liblas LIBLAS::las)
  message(STATUS "Found LibLAS")
endif ()

#--------------------------------------
# Eigen dependency - required
#--------------------------------------
find_package(Eigen3 REQUIRED)
check_depedency_target(Eigen3 Eigen3::Eigen)
print_version(Eigen3)

#--------------------------------------
# Yaml dependency - required
#--------------------------------------
find_package(yaml-cpp REQUIRED)
add_library(YAML::yamlcpp ALIAS yaml-cpp)
check_depedency_target(yaml-cpp YAML::yamlcpp)
print_version(yaml-cpp)

#--------------------------------------
# Boost dependency - required (note: boost is also found by ParaView)
#--------------------------------------
find_package(Boost REQUIRED COMPONENTS atomic filesystem program_options system thread)

#--------------------------------------
# PCL dependency - optional
#--------------------------------------
if (LIDARVIEW_USE_PCL)
  find_package(PCL REQUIRED COMPONENTS common kdtree features registration io sample_consensus)
  # WIP not clean contains a NIP "-Dno-qhull"
  add_definitions(${PCL_DEFINITIONS})
  print_version(PCL)
endif ()

#--------------------------------------
# Ceres dependency - optional
#--------------------------------------
if (LIDARVIEW_USE_CERES)
  find_package(Ceres REQUIRED)
endif ()

#--------------------------------------
# nanoflann dependency - optional
#--------------------------------------
if (LIDARVIEW_USE_NANOFLANN)
  find_package(nanoflann REQUIRED)
  print_version(nanoflann)
endif ()

#--------------------------------------
# Opencv dependency - optional
#--------------------------------------
if (LIDARVIEW_USE_OPENCV)
  find_package(OpenCV REQUIRED)
  print_version(OpenCV)
endif ()

#--------------------------------------
# LidarSlam dependency - optional
#--------------------------------------

if (PARAVIEW_PLUGIN_ENABLE_LidarSlam)
  find_package(LidarSlam REQUIRED QUIET)
  print_version(LidarSlam)
  check_depedency_target("LidarSlam::paraview_wrapping" LidarSlam::paraview_wrapping)
endif ()
