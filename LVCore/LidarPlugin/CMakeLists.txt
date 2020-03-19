project(LidarPlugin)
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

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

  include_directories(${SYSTEM_OPTION} ${VTK_INCLUDE_DIRS})
  if(PARAVIEW_BUILD_QT_GUI)
    include(${QT_USE_FILE})
    include_directories(${SYSTEM_OPTION} ${PARAVIEW_GUI_INCLUDE_DIRS})
  endif(PARAVIEW_BUILD_QT_GUI)

else(ParaView_SOURCE_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})

endif(ParaView_SOURCE_DIR)
vtk_module_load(vtklibproj4)


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
find_package(Boost REQUIRED COMPONENTS system thread chrono regex date_time filesystem program_options iostreams)
include_directories(${SYSTEM_OPTION} ${Boost_INCLUDE_DIRS})
set(ALL_BOOST_LIBRARIES ${Boost_LIBRARIES})
find_package(Boost OPTIONAL_COMPONENTS atomic timer)
include_directories(${SYSTEM_OPTION} ${Boost_INCLUDE_DIRS})
list(APPEND ALL_BOOST_LIBRARIES ${Boost_LIBRARIES})

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


# sources that will be wrap by paraview
# this sources will also be wrap in python in another plugin
# WARNING in case of inheritance, put also the upper classes
list(APPEND servermanager_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/vtkLidarProvider.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/vtkLidarReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/vtkLidarStream.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/vtkLidarPacketInterpreter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/KITTIDataSet/vtkLidarKITTIDataSetReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Applanix/vtkApplanixPositionReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/ArduPilotDataFlashLogReader/vtkArduPilotDataFlashLogReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkTemporalTransformsReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkTemporalTransformsWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkLASFileWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkBoundingBoxReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/MotionDetector/vtkMotionDetector.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/BirdEyeViewSnap/vtkBirdEyeViewSnap.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CameraProjector/vtkCameraProjector.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CameraMapper/vtkCameraMapper.cxx
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

if (Ceres_FOUND)
  list(APPEND servermanager_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CalibrationFromPoses/vtkCalibrationFromPoses.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TemporalTransformsRemapper/vtkTemporalTransformsRemapper.cxx
    )
endif(Ceres_FOUND)

if (PCL_FOUND)
  list(APPEND servermanager_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/PCLRansacModel/vtkPCLRansacModel.cxx
    )
endif(PCL_FOUND)

option(ENABLE_old_slam "Build old Lidar Slam (requires PCL, Ceres, nanoflann, ...)" ON)
if (ENABLE_old_slam AND PCL_FOUND AND Ceres_FOUND AND nanoflann_FOUND)
  list(APPEND servermanager_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Slam/vtkSlam.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Slam/vtkSpinningSensorKeypointExtractor.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Slam/vtkSlamManager.cxx
    )
  list(APPEND sources_which_do_not_inherit_from_vtkObject
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Slam/Slam.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Slam/SpinningSensorKeypointExtractor.cxx
    )
endif(ENABLE_old_slam AND PCL_FOUND AND Ceres_FOUND AND nanoflann_FOUND)

if (OpenCV_FOUND)
  list(APPEND servermanager_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/IO/Camera/vtkPCAPImageReader.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/IO/Camera/vtkOpenCVVideoReader.cxx
    )
  list(APPEND servermanager_xml
    xml/PCAPImageReader.xml
    )
  list(APPEND sources_which_do_not_inherit_from_vtkObject
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkOpenCVConversions.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkOpenCVConversions.cxx
    )
endif(OpenCV_FOUND)

# xml file that describe the proxy of the wrap sources in paraview
list(APPEND servermanager_xml
  xml/Lidar.xml
  xml/LidarKITTIDataSetReader.xml
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
  xml/ProcessingSample.xml
  xml/CameraProjector.xml
  xml/CameraMapper.xml
  xml/GridSource.xml
  xml/TemporalTransformsReader.xml
  xml/TemporalTransformsWriter.xml
  xml/TemporalTransformsApplier.xml
  xml/TemporalTransformsRemapper.xml
  xml/LASFileWriter.xml
  xml/OpenCVVideoReader.xml
  )

if (PCL_FOUND)
  list(APPEND servermanager_xml
    xml/PCLRansacModel.xml
    )
endif(PCL_FOUND)

if (Ceres_FOUND)
  list(APPEND servermanager_xml
    xml/CalibrationFromPoses.xml
    )
endif(Ceres_FOUND)

if (ENABLE_old_slam AND PCL_FOUND AND Ceres_FOUND AND nanoflann_FOUND)
  list(APPEND servermanager_xml
    xml/Slam.xml
    )
endif(ENABLE_old_slam AND PCL_FOUND AND Ceres_FOUND AND nanoflann_FOUND)


if (nanoflann_FOUND)
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

# other source code than won't be wrap by paraview but are needed to create the plugin
# We make the distinction on that inherit from vtkObject as this one will be also wrap
# in python later on
set(sources_which_inherit_from_vtkObject
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkCustomTransformInterpolator.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkTemporalTransforms.cxx
  )
list(APPEND sources_which_do_not_inherit_from_vtkObject
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/CrashAnalysing.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/NetworkSource.cxx
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
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Slam/KalmanFilter.cxx
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
  )
if (PCL_FOUND)
  list(APPEND sources_which_do_not_inherit_from_vtkObject
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkPCLConversions.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/RegistrationTools.cxx
    )
endif(PCL_FOUND)
if (Ceres_FOUND)
  list(APPEND sources_which_do_not_inherit_from_vtkObject
      ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Geometric/vtkGeometricCalibration.cxx
      ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Camera/CameraCalibration.cxx
      ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Slam/MotionModel/MotionModel.cxx
      )
endif (Ceres_FOUND)
if (PCL_FOUND AND Ceres_FOUND)
  list(APPEND sources_which_do_not_inherit_from_vtkObject
      ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Slam/TrajectoryReoptimization.cxx
      )
endif(PCL_FOUND AND Ceres_FOUND)

if (PCL_FOUND AND Ceres_FOUND AND OpenCV_FOUND)
  list(APPEND sources_which_do_not_inherit_from_vtkObject
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/CameraLidar/MIDHOG.cxx
    )
endif(PCL_FOUND AND Ceres_FOUND AND OpenCV_FOUND)

# plugin dependencies
list(APPEND deps
  ${PCAP_LIBRARY}
  ${liblas_LIBRARY}
  ${ALL_BOOST_LIBRARIES}
  "vtkIOInfovis" # https://public.kitware.com/pipermail/paraview/2016-January/036010.html
  ${vtklibproj4_LIBRARIES}
  ${PCL_LIBRARIES}
  ${CERES_LIBRARIES}
  ${OpenCV_LIBRARIES}
  ${YAML_CPP_LIBRARIES}
  )

message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
# folder where to look for header file
set(plugin_include_dirs
  ${PCAP_INCLUDE_DIR}
  ${EIGEN_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${vtklibproj4_INCLUDE_DIRS}
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
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/KITTIDataSet
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Camera
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Applanix
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/ArduPilotDataFlashLogReader/
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common
  ${CMAKE_CURRENT_SOURCE_DIR}/IO
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CalibrationFromPoses
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/MotionDetector
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/BirdEyeViewSnap
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CameraProjector
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CameraMapper
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
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Slam
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Slam/MotionModel
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TemporalTransformsRemapper
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/Grid
  )

# create pluging
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
install(FILES ${PROJECT_BINARY_DIR}/lidarplugin_export.h DESTINATION ${VV_INSTALL_LIBRARY_DIR})
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
SET(CMAKE_INSTALL_RPATH "$ORIGIN/../${VV_INSTALL_LIBRARY_DIR}")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#-----------------------------------------------------------------------------
# Build PacketFileSender target which enable to playback a recorded pcap on the network
#-----------------------------------------------------------------------------

add_executable(PacketFileSender StandAloneTools/PacketFileSender.cxx)
target_include_directories(PacketFileSender PRIVATE Common/Network)
target_link_libraries(PacketFileSender LINK_PUBLIC LidarPlugin ${ALL_BOOST_LIBRARIES})
if(WIN32)
  target_compile_definitions(PacketFileSender PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
endif(WIN32)

#add_executable(PCAPTester StandAloneTools/PCAPTester.cxx)
#target_include_directories(LidarPlugin PRIVATE ${plugin_include_dirs})
#target_link_libraries(PCAPTester LINK_PUBLIC LidarPlugin ${ALL_BOOST_LIBRARIES})
#if(WIN32)
#  target_compile_definitions(PCAPTester PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
#endif(WIN32)

#-----------------------------------------------------------------------------
# Build StandAlone targets which provide different thirparty tools
#-----------------------------------------------------------------------------
if (OpenCV_FOUND AND PCL_FOUND)
  add_executable(BBoxFromImagesDetections StandAloneTools/BBoxFromImagesDetections.cxx)
  target_include_directories(BBoxFromImagesDetections PRIVATE ${plugin_include_dirs})
  target_link_libraries(BBoxFromImagesDetections LINK_PUBLIC LidarPlugin ${ALL_BOOST_LIBRARIES})
  if(WIN32)
    target_compile_definitions(BBoxFromImagesDetections PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
  endif(WIN32)

  add_executable(LabelCloudFromImagesDetections StandAloneTools/LabelCloudFromImagesDetections.cxx)
  target_include_directories(LabelCloudFromImagesDetections PRIVATE ${plugin_include_dirs})
  target_link_libraries(LabelCloudFromImagesDetections LINK_PUBLIC LidarPlugin ${ALL_BOOST_LIBRARIES})
  if(WIN32)
    target_compile_definitions(LabelCloudFromImagesDetections PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
  endif(WIN32)

  add_executable(PostProcessLabelCloud StandAloneTools/PostProcessLabelCloud.cxx)
  target_include_directories(PostProcessLabelCloud PRIVATE ${plugin_include_dirs})
  target_link_libraries(PostProcessLabelCloud LINK_PUBLIC LidarPlugin ${ALL_BOOST_LIBRARIES})
  if(WIN32)
    target_compile_definitions(PostProcessLabelCloud PRIVATE -DWIN32 -DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)
  endif(WIN32)
endif (OpenCV_FOUND AND PCL_FOUND)


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
#  PCAPTester
  )

if (OpenCV_FOUND AND PCL_FOUND)
  list(APPEND executables_to_install
    BBoxFromImagesDetections
    LabelCloudFromImagesDetections
    PostProcessLabelCloud
    )
endif (OpenCV_FOUND AND PCL_FOUND)

if (APPLE)

  # install libraries
  install(TARGETS ${libraries_to_install}
          RUNTIME DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Libraries
          LIBRARY DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Libraries
          COMPONENT Runtime)

  # install executables
  install(TARGETS ${executables_to_install}
          RUNTIME DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/bin
          LIBRARY DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Libraries
          COMPONENT Runtime)

  # install paraview python modules
  install(DIRECTORY ${ParaView_DIR}/lib/site-packages/paraview DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Python)
  file(GLOB python_libs ${ParaView_DIR}/lib/*Python.so)
  install(FILES ${python_libs} DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Libraries)

  # # install some pdf
  # install(FILES ${LidarView_SOURCE_DIR}/Documentation/LidarView_Developer_Guide.pdf DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Resources)
  # configure_file(${LidarView_SOURCE_DIR}/Documentation/LidarView_Developer_Guide.pdf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOFTWARE_NAME}.app/Contents/Resources/LidarView_Developer_Guide.pdf)

  # install(FILES ${LidarView_SOURCE_DIR}/Documentation/LidarView_User_Guide.pdf DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Resources)
  # configure_file(${LidarView_SOURCE_DIR}/Documentation/LidarView_User_Guide.pdf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOFTWARE_NAME}.app/Contents/Resources/LidarView_User_Guide.pdf)

else()
  # install libraries
  install(TARGETS ${libraries_to_install}
          RUNTIME DESTINATION ${VV_INSTALL_RUNTIME_DIR}
          LIBRARY DESTINATION ${VV_INSTALL_LIBRARY_DIR}
          COMPONENT Runtime)

  # install executables
  install(TARGETS ${executables_to_install}
          RUNTIME DESTINATION ${VV_INSTALL_RUNTIME_DIR}
          LIBRARY DESTINATION ${VV_INSTALL_LIBRARY_DIR}
          COMPONENT Runtime)
endif()

#-----------------------------------------------------------------------------
# Option to build some tests
#-----------------------------------------------------------------------------

option(BUILD_TESTING "Build some unitary test" OFF)
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
