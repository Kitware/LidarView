project(LidarPlugin)
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
include(CMakeDependentOption)

set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ standard")
message(STATUS "Building LidarPlugin with C++${CMAKE_CXX_STANDARD} standard")

#-----------------------------------------------------------------------------
# Temporary Fix
#-----------------------------------------------------------------------------
message(AUTHOR_WARNING "Until the 'Speedup interpolator' merge request is integrated to VTK, we provide the patch")
set(interpolator_pach_until_vtk_update
  vtkPatch/vtkCustomPiecewiseFunction.cxx
  vtkPatch/vtkCustomQuaternionInterpolator.cxx
  vtkPatch/vtkCustomTupleInterpolator.cxx
)

#-----------------------------------------------------------------------------
# Option to hide/show external warning library
#-----------------------------------------------------------------------------

set(SYSTEM_OPTION)
option(BUILD_HIDE_WARNING_FROM_EXTERNAL_DEPENDENCIES "hide external libraries warning, by handling them as system libraries" ON)
if(BUILD_HIDE_WARNING_FROM_EXTERNAL_DEPENDENCIES)
  set(SYSTEM_OPTION SYSTEM)
endif()

#-----------------------------------------------------------------------------
# Handle plugin library dependencies
#-----------------------------------------------------------------------------

#--------------------------------------
# ParaView dependency
#--------------------------------------
if(ParaView_SOURCE_DIR)

  if(NOT VTK_INCLUDE_DIRS )
    message(FATAL_ERROR "VTK_INCLUDE_DIRS not set or empty")
  endif()
  include_directories(${SYSTEM_OPTION} ${VTK_INCLUDE_DIRS})

  if(PARAVIEW_BUILD_QT_GUI)
    if(NOT QT_USE_FILE OR NOT PARAVIEW_GUI_INCLUDE_DIRS)
      message(FATAL_ERROR "QT_USE_FILE OR PARAVIEW_GUI_INCLUDE_DIRS not set or empty")
    endif()
    include(${QT_USE_FILE})
    include_directories(${SYSTEM_OPTION} ${PARAVIEW_GUI_INCLUDE_DIRS})
  endif(PARAVIEW_BUILD_QT_GUI)

else(ParaView_SOURCE_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})

endif(ParaView_SOURCE_DIR)

#--------------------------------------
# vtklibproj module
#--------------------------------------
vtk_module_load(vtklibproj)


#--------------------------------------
# pcap dependency
#--------------------------------------
find_library(PCAP_LIBRARY pcap DOC "pcap library")
find_path(PCAP_INCLUDE_DIR pcap.h DOC "pcap include directory")
mark_as_advanced(PCAP_LIBRARY PCAP_INCLUDE_DIR)
include_directories(${SYSTEM_OPTION} ${PCAP_INCLUDE_DIR})

#--------------------------------------
# liblas dependency
#--------------------------------------
set(LASNAME las)
if(WIN32)
  set(LASNAME liblas)
endif()

find_library(liblas_LIBRARY ${LASNAME} DOC "liblas library")
find_path(liblas_INCLUDE_DIR liblas/version.hpp DOC "liblas include directory")
mark_as_advanced(liblas_LIBRARY liblas_INCLUDE_DIR)
include_directories(${SYSTEM_OPTION} ${liblas_INCLUDE_DIR})

#--------------------------------------
# Eigen dependency
#--------------------------------------
find_package(Eigen REQUIRED)
#include_directories(${SYSTEM_OPTION} ${EIGEN_INCLUDE_DIR})

#--------------------------------------
# Yaml dependency
#--------------------------------------
find_package(yaml-cpp REQUIRED)
include_directories(${SYSTEM_OPTION} ${YAML_CPP_INCLUDE_DIR})

#--------------------------------------
# PCL dependency
#--------------------------------------
option(ENABLE_pcl "PCL will be required for filters using pcl (PCLRansacFilter, Slam, ...)" OFF)
if (ENABLE_pcl)
  find_package(PCL REQUIRED)
  include_directories(${SYSTEM_OPTION} ${PCL_INCLUDE_DIRS})
  add_definitions(${PCL_DEFINITIONS})

  # Since PCL 1.9, PCL_DEFINITIONS is deprecated as options are directly
  # included in targets or in PCL_COMPILE_OPTIONS.
  # We need to check if some flags were used by PCL as they could build a
  # different version of Eigen, not compatible with default one.
  list(APPEND pcl_compile_flags ${PCL_DEFINITIONS} ${PCL_COMPILE_OPTIONS})
  string(FIND "${pcl_compile_flags}" "-march=native" pcl_use_march_native)
  if (${pcl_use_march_native} GREATER -1)
    message(WARNING "PCL assumes '-march=native' flag was used, which may enable AVX optimizations.\n"
                    "Please ensure that other libs using Eigen are also compiled with the same SSE/AVX flags, "
                    "as it could change memory alignement between these versions. Otherwise, the program could "
                    "segfault allocating/freeing dynamic objects. See here for more info : "
                    "https://github.com/ethz-asl/eigen_catkin/wiki/Eigen-Memory-Issues#mismatched-build-flags")
  endif()
endif(ENABLE_pcl)

#--------------------------------------
# Boost dependency
#--------------------------------------
set(Boost_USE_MULTITHREADED ON)
# the expected behavior of find_package when  used wiht REQUIRED and OPTIONAL_COMPONENTS
# it not the one expected, as it also required the optional component...
find_package(Boost REQUIRED COMPONENTS system thread filesystem program_options iostreams)
include_directories(${SYSTEM_OPTION} ${Boost_INCLUDE_DIRS})

#--------------------------------------
# Ceres dependency
#--------------------------------------
option(ENABLE_ceres "Ceres is required for filters using non-linear least square optimization (Slam, autocalibration, ...)" OFF)
if (ENABLE_ceres)
  find_package(Ceres REQUIRED)
  include_directories(${SYSTEM_OPTION} ${CERES_INCLUDE_DIRS})
endif(ENABLE_ceres)

#--------------------------------------
# Opencv dependency
#--------------------------------------
option(ENABLE_opencv "OpenCV is required for handling lidar-camera multisensor systems" OFF)
if (ENABLE_opencv)
  find_package(OpenCV REQUIRED)
  include_directories(${SYSTEM_OPTION} ${OpenCV_INCLUDE_DIRS})
endif(ENABLE_opencv)

#--------------------------------------
# nanoflann dependency
#--------------------------------------
option(ENABLE_nanoflann "Nanoflann will be required for filters using some optimized kdtree (SLAM, DBSCAN, ...)" OFF)
  if (ENABLE_nanoflann)
  # because nanoflann use modern C++, no need to specify the include dir or anything else
  # you just need to link against the target nanoflann::nanoflann
  find_package(nanoflann REQUIRED)
  list(APPEND deps nanoflann::nanoflann)
endif(ENABLE_nanoflann)

#-----------------------------------------------------------------------------
# Build Paraview Plugin
#-----------------------------------------------------------------------------
# Set Variables
set(servermanager_sources)
set(sources_which_inherit_from_vtkObject)
set(sources_which_do_not_inherit_from_vtkObject)
set(servermanager_xml)
set(plugin_include_dirs)

# Old Slam sources
# ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization/KalmanFilter.cxx
# ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization/MotionModel/MotionModel.cxx
# ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization/TrajectoryReoptimization.cxx
# ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization
# ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization/MotionModel

# Optional Sources
# Built by default if dependencies are enabled, option available to disable them

# CalibrationFromPoses
cmake_dependent_option(LIDARVIEW_BUILD_CALIBFROMPOSES "Build CalibrationFromPoses" ON "Ceres_FOUND" OFF)
if(LIDARVIEW_BUILD_CALIBFROMPOSES)
  list(APPEND servermanager_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CalibrationFromPoses/vtkCalibrationFromPoses.cxx )
  list(APPEND servermanager_xml
    xml/CalibrationFromPoses.xml )
  list(APPEND plugin_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CalibrationFromPoses )

  list(APPEND servermanager_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TemporalTransformsRemapper/vtkTemporalTransformsRemapper.cxx
    )
  list(APPEND sources_which_do_not_inherit_from_vtkObject
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Geometric/vtkGeometricCalibration.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Camera/CameraCalibration.cxx
    )
endif()

# OpenCV extensions
cmake_dependent_option(LIDARVIEW_BUILD_OPENCVEXTENSIONS "Build OpenCV extensions" ON "OpenCV_FOUND" OFF)
if (LIDARVIEW_BUILD_OPENCVEXTENSIONS)
  list(APPEND servermanager_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/IO/Camera/vtkPCAPImageReader.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/IO/Camera/vtkOpenCVVideoReader.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/OpenCVImageMapper/vtkOpenCVImageMapper.cxx
    )
  list(APPEND servermanager_xml
    xml/PCAPImageReader.xml
    xml/OpenCVImageMapper.xml
    )
  list(APPEND sources_which_do_not_inherit_from_vtkObject
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkOpenCVConversions.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkOpenCVConversions.cxx
    )
endif()

# CloudTools
cmake_dependent_option(LIDARVIEW_BUILD_CLOUDTOOLS "Build CloudTools" ON "nanoflann_FOUND" OFF)
if (LIDARVIEW_BUILD_CLOUDTOOLS)
  list(APPEND servermanager_sources
       ${CMAKE_CURRENT_SOURCE_DIR}/Filter/SeparateCloudKnn/vtkSeparateCloudKnn.cxx
       ${CMAKE_CURRENT_SOURCE_DIR}/Filter/DBSCANClustering/vtkDBSCANClustering.cxx
       )
  list(APPEND servermanager_xml
       xml/SeparateCloudKnn.xml
       xml/DBSCANClustering.xml
       )
  list(APPEND sources_which_do_not_inherit_from_vtkObject
       ${CMAKE_CURRENT_SOURCE_DIR}/Common/SegmentedCloudTransformations.cxx
       )
endif()

# RegistrationTools
cmake_dependent_option(LIDARVIEW_BUILD_REGTOOLS "Build RegistrationTools" ON "PCL_FOUND;Ceres_FOUND" OFF)
if (LIDARVIEW_BUILD_REGTOOLS)
  list(APPEND sources_which_do_not_inherit_from_vtkObject
      ${CMAKE_CURRENT_SOURCE_DIR}/Common/RegistrationTools.cxx )
endif()

# MIDHOG Filter extensions
cmake_dependent_option(LIDARVIEW_BUILD_MIDHOG "Build MIDHOG Filter" ON "PCL_FOUND;Ceres_FOUND;OpenCV_FOUND" OFF)
if (LIDARVIEW_BUILD_MIDHOG)
  list(APPEND sources_which_do_not_inherit_from_vtkObject
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/CameraLidar/MIDHOG.cxx )
endif()

# PCL Miscs
cmake_dependent_option(LIDARVIEW_BUILD_PCLMISCS "Build PCL Miscs" ON "PCL_FOUND" OFF)
if (LIDARVIEW_BUILD_PCLMISCS)
  list(APPEND servermanager_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/PCLRansacModel/vtkPCLRansacModel.cxx )
  list(APPEND servermanager_xml
    xml/PCLRansacModel.xml )

  list(APPEND sources_which_do_not_inherit_from_vtkObject
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkPCLConversions.cxx
    )
endif()

# LidarPlugin default wrapped sources
# those will be wrapped into paraview proxies and in Python
# WARNING in case of inheritance, put also the upper classes
list(APPEND servermanager_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/vtkLidarReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/vtkLidarStream.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/vtkLidarPacketInterpreter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Applanix/vtkApplanixPositionReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/ArduPilotDataFlashLogReader/vtkArduPilotDataFlashLogReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common/vtkPositionOrientationPacketInterpreter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common/vtkPositionOrientationPacketReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common/vtkPositionOrientationStream.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkStream.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkInterpreter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkTemporalTransformsReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkTemporalTransformsWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkLASFileWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkPCDWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkBoundingBoxReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/MotionDetector/vtkMotionDetector.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/BirdEyeViewSnap/vtkBirdEyeViewSnap.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CameraProjector/vtkCameraProjector.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CameraMapper/vtkCameraMapper.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/PlaneProjector/vtkPlaneProjector.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/LidarRawSignalImage/vtkLidarRawSignalImage.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/PointCloudLinearProjector/vtkPointCloudLinearProjector.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/LaplacianInfilling/vtkLaplacianInfilling.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/MLSPosesSmoothing/vtkMLSPosesSmoothing.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/ProcessingSample/vtkProcessingSample.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Ransac/vtkRansacPlaneModel.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TemporalTransformsApplier/vtkTemporalTransformsApplier.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrailingFrame/vtkTrailingFrame.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/Grid/vtkGridSource.cxx
  )

# LidarPlugin default xml proxy descriptions
list(APPEND servermanager_xml
  xml/Reader.xml
  xml/Stream.xml
  xml/Interpreter.xml
  xml/ApplanixPositionReader.xml
  xml/ArduPilotDataFlashLogReader.xml
  xml/BoundingBoxReader.xml
  xml/MotionDetector.xml
  xml/BirdEyeViewSnap.xml
  xml/LidarRawSignalImage.xml
  xml/PointCloudLinearProjector.xml
  xml/LaplacianInfilling.xml
  xml/MLSPosesSmoothing.xml
  xml/RansacPlaneModel.xml
  xml/TrailingFrame.xml
  xml/PCDWriter.xml
  xml/ProcessingSample.xml
  xml/CameraProjector.xml
  xml/CameraMapper.xml
  xml/PlaneProjector.xml
  xml/GridSource.xml
  xml/TemporalTransformsReader.xml
  xml/TemporalTransformsWriter.xml
  xml/TemporalTransformsApplier.xml
  xml/TemporalTransformsRemapper.xml
  xml/LASFileWriter.xml
  xml/OpenCVVideoReader.xml
  )

# LidarPlugin default non-wrapped sources
list(APPEND sources_which_inherit_from_vtkObject
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkCustomTransformInterpolator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkTemporalTransforms.cxx
  )
list(APPEND sources_which_do_not_inherit_from_vtkObject
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/CrashAnalysing.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/PacketReceiver.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/PacketFileWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/PacketConsumer.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/FramesSeriesUtils.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Network/NetworkPacket.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common/NMEAParser.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common/GPSProjectionUtils.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkLASFileWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/LASFileWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/CategoriesConfig.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/MotionDetector/vtkSphericalMap.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Network/vtkPacketFileWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Network/vtkPacketFileReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Network/vvPacketSender.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkEigenTools.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/CameraProjection.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Camera/CameraModel.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/${interpolator_pach_until_vtk_update}
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkConversions.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Temporal/vtkTimeCalibration.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Geometric/vtkCarGeometricCalibration.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkPipelineTools.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkHelper.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/LVTime.cxx
  )

# LidarPlugin default dependencies
list(APPEND deps
  ${PCAP_LIBRARY}
  ${liblas_LIBRARY}
  ${Boost_LIBRARIES}
  "vtkIOInfovis" # https://public.kitware.com/pipermail/paraview/2016-January/036010.html
  ${vtklibproj_LIBRARIES}
  ${PCL_LIBRARIES}
  ${CERES_LIBRARIES}
  ${OpenCV_LIBRARIES}
  ${YAML_CPP_LIBRARIES}
  )

# LidarPlugin default Include Directories
list(APPEND plugin_include_dirs
  ${PCAP_INCLUDE_DIR}
  ${EIGEN_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${vtklibproj_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Common
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/ML
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Network
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Camera
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Geometric
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Temporal
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/CameraLidar
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/NanoflannAdaptor
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Camera
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Camera
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Applanix
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/ArduPilotDataFlashLogReader/
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common
  ${CMAKE_CURRENT_SOURCE_DIR}/IO
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/MotionDetector
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/BirdEyeViewSnap
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CameraProjector
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CameraMapper
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/OpenCVImageMapper
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/PlaneProjector
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/DBSCANClustering
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/LidarRawSignalImage
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/PointCloudLinearProjector
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/LaplacianInfilling
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/MLSPosesSmoothing
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Ransac
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrailingFrame
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/SeparateCloudKnn
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TemporalTransformsApplier
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/ProcessingSample
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/PCLRansacModel
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TemporalTransformsRemapper
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/Grid
  )

# LidarPlugin Plugin Creation
add_paraview_plugin(LidarPlugin "1.0"
  SERVER_MANAGER_SOURCES ${servermanager_sources}
  SERVER_MANAGER_XML ${servermanager_xml}
  SERVER_SOURCES ${sources_which_inherit_from_vtkObject} ${sources_which_do_not_inherit_from_vtkObject}
  # GUI_SOURCES ${gui_sources}
  REQUIRED_ON_SERVER)
target_include_directories(LidarPlugin PUBLIC ${plugin_include_dirs})
target_link_libraries(LidarPlugin PUBLIC ${deps})
include(GenerateExportHeader)
generate_export_header(LidarPlugin)
install(FILES ${PROJECT_BINARY_DIR}/lidarplugin_export.h DESTINATION ${LV_INSTALL_LIBRARY_DIR})
target_include_directories(LidarPlugin PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
if(WIN32)
  # -DBOOST_PROGRAM_OPTIONS_DYN_LINK is necessary to use boost as a dynamic library
  # more about it here: https://svn.boost.org/trac10/ticket/13326
  target_compile_definitions(LidarPlugin PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
endif(WIN32)
if (APPLE)
  target_compile_definitions(LidarPlugin PRIVATE -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
endif(APPLE)

# The main application executable uses a "forward exectuable"
# that sets LD_LIBRARY_PATH to the appropriate location before calling the
# original executable, so that dynamic libraries can be found.
# For the executanles of the standalone tools it is simpler to modify rpaths:
set(CMAKE_SKIP_RPATH FALSE)
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "$ORIGIN/../${LV_INSTALL_LIBRARY_DIR}")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#-----------------------------------------------------------------------------
# Build PacketFileSender target which enable to playback a recorded pcap on the network
#-----------------------------------------------------------------------------

add_executable(PacketFileSender StandAloneTools/PacketFileSender.cxx)
target_include_directories(PacketFileSender PRIVATE Common/Network)
target_link_libraries(PacketFileSender LINK_PUBLIC LidarPlugin ${Boost_LIBRARIES})
if(WIN32)
  target_compile_definitions(PacketFileSender PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
endif(WIN32)


#-----------------------------------------------------------------------------
# Build PCAPSeparator target which enable to split a recorded pcap on two
#-----------------------------------------------------------------------------

add_executable(PCAPSeparator StandAloneTools/PCAPSeparator.cxx)
target_link_libraries(PCAPSeparator LINK_PUBLIC LidarPlugin ${ALL_BOOST_LIBRARIES})
if(WIN32)
  target_compile_definitions(PCAPSeparator PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
endif(WIN32)

#add_executable(PCAPTester StandAloneTools/PCAPTester.cxx)
#target_include_directories(LidarPlugin PRIVATE ${plugin_include_dirs})
#target_link_libraries(PCAPTester LINK_PUBLIC LidarPlugin ${Boost_LIBRARIES})
#if(WIN32)
#  target_compile_definitions(PCAPTester PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
#endif(WIN32)

#-----------------------------------------------------------------------------
# Build StandAlone targets which provide different thirparty tools
#-----------------------------------------------------------------------------
# ImageDetections
cmake_dependent_option(LIDARVIEW_BUILD_IMGDETECTIONS "Build ImageDetections" ON "PCL_FOUND;OpenCV_FOUND" OFF)
if (LIDARVIEW_BUILD_IMGDETECTIONS)
  add_executable(BBoxFromImagesDetections StandAloneTools/BBoxFromImagesDetections.cxx)
  target_include_directories(BBoxFromImagesDetections PRIVATE ${plugin_include_dirs})
  target_link_libraries(BBoxFromImagesDetections LINK_PUBLIC LidarPlugin ${Boost_LIBRARIES})
  if(WIN32)
    target_compile_definitions(BBoxFromImagesDetections PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
  endif(WIN32)

  add_executable(LabelCloudFromImagesDetections StandAloneTools/LabelCloudFromImagesDetections.cxx)
  target_include_directories(LabelCloudFromImagesDetections PRIVATE ${plugin_include_dirs})
  target_link_libraries(LabelCloudFromImagesDetections LINK_PUBLIC LidarPlugin ${Boost_LIBRARIES})
  if(WIN32)
    target_compile_definitions(LabelCloudFromImagesDetections PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
  endif(WIN32)

endif()

# PostProcessLabelCloud
cmake_dependent_option(LIDARVIEW_BUILD_POSTPROCLABELCLOUD "Build PostProcessLabelCloud" ON "PCL_FOUND;OpenCV_FOUND;nanoflann_FOUND" OFF)
if (LIDARVIEW_BUILD_POSTPROCLABELCLOUD)
  add_executable(PostProcessLabelCloud StandAloneTools/PostProcessLabelCloud.cxx)
  target_include_directories(PostProcessLabelCloud PRIVATE ${plugin_include_dirs})
  target_link_libraries(PostProcessLabelCloud LINK_PUBLIC LidarPlugin ${Boost_LIBRARIES})
  if(WIN32)
    target_compile_definitions(PostProcessLabelCloud PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
  endif(WIN32)
endif()


#-----------------------------------------------------------------------------
# This code is here for LEGACY REASON and should NOT BE USED and even REMOVE at some point !!!!
# It's actually nice to expose the classes in python, but the more freedom we have
# the more dirty the code get...
# Create a static and dynamic python target named LidarPluginPython and
# (LidarPluginPythonD that wrap all the classes that inherit from vtk in python.
# When a paraview plugin is created, you only have acces to the Filter exposed
# with a proxy, and you can but SHOULDN'T used the method GetClientSideObject()
# to have acces to the vtkObject directly.
# With what is done here you have acces to all the vtk inherit  C++ code, even if
# it's not exposed in paraview.
# Simply used "import LidarPluginPython" and you have acces to the
# wrapped c++ code
#-----------------------------------------------------------------------------

# needed as we can't use target_include_directories because the target are not defined yet
include_directories(${plugin_include_dirs})
set(towrap
  ${servermanager_sources}
  ${sources_which_inherit_from_vtkObject}
  )
# the quoted "" are needed otherwise only the first file of the list will be process
WRAP_PLUGIN_FOR_PYTHON(LidarPlugin "${towrap}" "NOTTHERE")

#-----------------------------------------------------------------------------
# Install targets and ressources
#-----------------------------------------------------------------------------

set(libraries_to_install
  LidarPlugin
  LidarPluginPython
  LidarPluginPythonD
  )

set(executables_to_install
  PacketFileSender
  PCAPSeparator
#  PCAPTester
  )

if (LIDARVIEW_BUILD_IMGDETECTIONS)
  list(APPEND executables_to_install
    BBoxFromImagesDetections
    LabelCloudFromImagesDetections
    )
endif()

if (LIDARVIEW_BUILD_POSTPROCLABELCLOUD)
  list(APPEND executables_to_install
    PostProcessLabelCloud
    )
endif()

# This is normal Superbuild operation, on APPLE the Superbuild's
# bundling script will only pick from LV_INSTALL_LIBRARY_DIR,
# external dependencies have to be moved manually from their
# custom install directories to the MacOS hierarchy
# It also enable us to filter more finely what ends up in the bundle
if (APPLE)

  # install libraries
  install(TARGETS ${libraries_to_install}
          RUNTIME DESTINATION ${LV_INSTALL_LIBRARY_DIR}
          LIBRARY DESTINATION ${LV_INSTALL_LIBRARY_DIR}
          COMPONENT Runtime)

  # install some dependencies for packaging
  install(FILES ${PCAP_LIBRARY} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  install(FILES ${liblas_LIBRARY} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  install(FILES ${Boost_LIBRARIES} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  FILE(GLOB flann_libraries ${CMAKE_INSTALL_PREFIX}/lib/libflann*)
  FILE(GLOB pcl_libraries ${CMAKE_INSTALL_PREFIX}/lib/libpcl*)
  install(FILES ${flann_libraries} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  install(FILES ${pcl_libraries} DESTINATION ${LV_INSTALL_LIBRARY_DIR})

  # install executables
  install(TARGETS ${executables_to_install}
          RUNTIME DESTINATION ${LV_INSTALL_LIBRARY_DIR}/../bin
          LIBRARY DESTINATION ${LV_INSTALL_LIBRARY_DIR}
          COMPONENT Runtime)

  # install paraview python modules
  install(DIRECTORY ${ParaView_DIR}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages/ DESTINATION ${LV_INSTALL_LIBRARY_DIR}/../Python)
  file(GLOB python_dylib ${ParaView_DIR}/lib/*Python*.dylib)
  install(FILES ${python_dylib} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  file(GLOB python_so ${ParaView_DIR}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages/vtkmodules/*Python.so)
  install(FILES ${python_so} DESTINATION ${LV_INSTALL_LIBRARY_DIR}/../Python/vtkmodules)

else()
  # TODO Address this undesired patch
  # install paraview libs in LV_INSTALL_LIBRARY_DIR
  # needed to run LidarView from install path
  if(UNIX)
    file(GLOB paraview_libs ${ParaView_DIR}/lib/lib*.so*)
    install(FILES ${paraview_libs} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  endif()

  # install libraries
  install(TARGETS ${libraries_to_install}
          RUNTIME DESTINATION "bin"
          LIBRARY DESTINATION ${LV_INSTALL_LIBRARY_DIR}
          COMPONENT Runtime)

  # install executables
  install(TARGETS ${executables_to_install}
          RUNTIME DESTINATION "bin"
          LIBRARY DESTINATION ${LV_INSTALL_LIBRARY_DIR}
          COMPONENT Runtime)
endif()

#-----------------------------------------------------------------------------
# Option to build some tests
#-----------------------------------------------------------------------------

option(BUILD_TESTING "Build some unitary test" OFF)
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
