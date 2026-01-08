list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}/Modules")

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
# ParaView dependency - required
#--------------------------------------
find_package(ParaView 6.0 REQUIRED)
set(paraview_version "${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}")
if (LIDARVIEW_USE_QT AND NOT PARAVIEW_USE_QT)
  message(FATAL_ERROR "PARAVIEW_USE_QT is OFF, Paraview must be built with Qt")
endif ()
if (LIDARVIEW_USE_PYTHON AND NOT PARAVIEW_USE_PYTHON)
  message(FATAL_ERROR "PARAVIEW_USE_PYTHON is OFF, Paraview must be built with Python")
endif ()
print_version(ParaView)

#--------------------------------------
# VTK (from ParaView package) dependency - required
#--------------------------------------
if (NOT VTK_FOUND OR NOT VTK_VERSION)
  message(FATAL_ERROR "VTK not found")
endif ()
print_version(VTK)

#--------------------------------------
# Qt dependency - optional
#--------------------------------------
if (LIDARVIEW_USE_QT)
  if (NOT DEFINED PARAVIEW_QT_MAJOR_VERSION)
    message(FATAL_ERROR "ParaView should provide PARAVIEW_QT_MAJOR_VERSION variable.")
  endif ()
  find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED COMPONENTS Core Widgets)
  print_version("Qt${PARAVIEW_QT_MAJOR_VERSION}")
endif ()

if (LIDARVIEW_USE_QT AND NOT LIDARVIEW_USE_PYTHON)
  message(FATAL_ERROR "If LidarView is built with Qt, LIDARVIEW_USE_PYTHON must be enabled.")
endif ()

#--------------------------------------
# Python dependency - optional without Qt
#--------------------------------------
if (LIDARVIEW_USE_PYTHON OR LIDARVIEW_USE_QT)
  find_package(Python3 3.10 QUIET REQUIRED COMPONENTS Interpreter)
  set(lidarview_python_version "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
  message(STATUS "Using Python ${lidarview_python_version}")
endif ()

#--------------------------------------
# Tins dependency - required
#--------------------------------------
find_package(Tins REQUIRED QUIET)
print_version(Tins)

#--------------------------------------
# Eigen dependency - required
#--------------------------------------
find_package(Eigen3 REQUIRED)
check_depedency_target(Eigen3 Eigen3::Eigen)
print_version(Eigen3)

#--------------------------------------
# Boost dependency - required (note: boost is also found by ParaView)
#--------------------------------------
find_package(Boost REQUIRED COMPONENTS atomic filesystem program_options system thread)

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
  find_package(nanoflann 1.5 REQUIRED)
  print_version(nanoflann)
endif ()

#--------------------------------------
# CURL dependency - optional
#--------------------------------------
if (LIDARVIEW_USE_CURL)
  find_package(CURL REQUIRED)
  if (CMAKE_MAJOR_VERSION GREATER_EQUAL "4")
    print_version(CURL)
  else ()
    message(STATUS "Found CURL: ${CURL_VERSION_STRING}")
  endif ()
endif ()

#--------------------------------------
# Opencv dependency - optional
#--------------------------------------
if (LIDARVIEW_USE_OPENCV)
  find_package(OpenCV REQUIRED)
  print_version(OpenCV)
endif ()

#--------------------------------------
# Yaml dependency - optional
#--------------------------------------
if (LIDARVIEW_USE_YAMLCPP)
  find_package(yaml-cpp REQUIRED)
  if (TARGET yaml-cpp)
    add_library(YAML::yamlcpp ALIAS yaml-cpp)
  elseif (TARGET yaml-cpp::yaml-cpp)
    add_library(YAML::yamlcpp ALIAS yaml-cpp::yaml-cpp)
  endif ()
  check_depedency_target(yaml-cpp YAML::yamlcpp)
  print_version(yaml-cpp)
endif ()

#--------------------------------------
# LidarSlam plugin dependency - optional
#--------------------------------------
if (LIDARVIEW_USE_LIDARSLAM_PLUGIN)
  find_package(LidarSlam REQUIRED QUIET)
  print_version(LidarSlam)
  check_depedency_target("LidarSlam::paraview_wrapping" LidarSlam::paraview_wrapping)
endif ()
