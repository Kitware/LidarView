set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ standard")
message(STATUS "Building LidarCore with C++${CMAKE_CXX_STANDARD} standard")

include(CMakeDependentOption)

#-----------------------------------------------------------------------------
# VTK Temporary Patches
#-----------------------------------------------------------------------------
#See https://gitlab.kitware.com/LidarView/lidarview-core/-/issues/36
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

# Helpers
function(include_directories_system scope include_dirs)
  include_directories(${SYSTEM_OPTION} ${scope} ${include_dirs})
endfunction(include_directories_system )

function(target_link_libraries_system target scope)
  set(libs ${ARGN})
  foreach(lib ${libs})
    get_target_property(lib_include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${target} ${SYSTEM_OPTION} ${scope} ${lib_include_dirs})
    target_link_libraries(${target} ${scope} ${lib})
  endforeach(lib)
endfunction(target_link_libraries_system )

#-----------------------------------------------------------------------------
# Handle plugin library dependencies
#-----------------------------------------------------------------------------
set(LIDARPLUGIN_DEPS_INCLUDE_DIRS)
set(LIDARPLUGIN_DEPS)

#--------------------------------------
# pcap dependency
#--------------------------------------
find_library(PCAP_LIBRARY pcap DOC "pcap library")
find_path(PCAP_INCLUDE_DIR pcap.h DOC "pcap include directory")
list(APPEND LIDARPLUGIN_DEPS_INCLUDE_DIRS ${PCAP_INCLUDE_DIR})
list(APPEND LIDARPLUGIN_DEPS              ${PCAP_LIBRARY})

#--------------------------------------
# liblas dependency
#--------------------------------------
set(LASNAME las)
if(WIN32)
  set(LASNAME liblas)
endif()

find_library(liblas_LIBRARY ${LASNAME} DOC "liblas library")
find_path(liblas_INCLUDE_DIR liblas/version.hpp DOC "liblas include directory")
add_library(liblas INTERFACE)
target_link_libraries     (liblas INTERFACE ${liblas_LIBRARY})
target_include_directories(liblas INTERFACE ${liblas_INCLUDE_DIR})
list(APPEND LIDARPLUGIN_DEPS              liblas)
#--------------------------------------
# Eigen dependency
#--------------------------------------
find_package(Eigen3 REQUIRED)
list(APPEND LIDARPLUGIN_DEPS              Eigen3::Eigen)
#wipwip things to check between eigen vs eigen3 from ceres etc

#--------------------------------------
# Yaml dependency
#--------------------------------------
find_package(yaml-cpp REQUIRED)
list(APPEND LIDARPLUGIN_DEPS              yaml-cpp )
if(NOT TARGET yaml-cpp)
  message(FATAL_ERROR "yaml-cpp not found")
endif()

#--------------------------------------
# Boost dependency
#--------------------------------------
find_package(Boost REQUIRED COMPONENTS system thread filesystem program_options iostreams)
list(APPEND LIDARPLUGIN_DEPS_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
list(APPEND LIDARPLUGIN_DEPS              ${Boost_LIBRARIES})

#--------------------------------------
# PCL dependency - optional
#--------------------------------------
option(ENABLE_pcl "PCL will be required for filters using pcl (PCLRansacFilter, Slam, ...)" OFF)
if (ENABLE_pcl)
  find_package(PCL REQUIRED COMPONENTS common kdtree registration io)
  
  #WIP not clean #contains a NIP "-Dno-qhull"
  add_definitions(${PCL_DEFINITIONS})
  
  list(APPEND LIDARPLUGIN_DEPS_INCLUDE_DIRS ${PCL_INCLUDE_DIRS}) #wip make it better
  # We cannot let PCL include VTK itself as it conflicts with how vtk_module does it,
  # This prevent build command too long on Win32
  list(FILTER PCL_LIBRARIES EXCLUDE REGEX "VTK::.*")
  list(APPEND LIDARPLUGIN_DEPS ${PCL_LIBRARIES} )
endif(ENABLE_pcl)

#--------------------------------------
# Ceres dependency - optional
#--------------------------------------
option(ENABLE_ceres "Ceres is required for filters using non-linear least square optimization (Slam, autocalibration, ...)" OFF)
if (ENABLE_ceres)
  find_package(Ceres REQUIRED)
  list(APPEND LIDARPLUGIN_DEPS_INCLUDE_DIRS ${CERES_INCLUDE_DIRS})
  list(APPEND LIDARPLUGIN_DEPS              ${CERES_LIBRARIES})
endif(ENABLE_ceres)

#--------------------------------------
# Opencv dependency - optional
#--------------------------------------
option(ENABLE_opencv "OpenCV is required for handling lidar-camera multisensor systems" OFF)
if (ENABLE_opencv)
  find_package(OpenCV REQUIRED)
  list(APPEND LIDARPLUGIN_DEPS_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
  list(APPEND LIDARPLUGIN_DEPS              ${OpenCV_LIBRARIES})
endif(ENABLE_opencv)

#--------------------------------------
# nanoflann dependency - optional
#--------------------------------------
option(ENABLE_nanoflann "Nanoflann will be required for filters using some optimized kdtree (SLAM, DBSCAN, ...)" OFF)
if (ENABLE_nanoflann)
  find_package(nanoflann REQUIRED)
  list(APPEND LIDARPLUGIN_DEPS nanoflann::nanoflann)
endif(ENABLE_nanoflann)

#-----------------------------------------------------------------------------
# Build Paraview Plugin
#-----------------------------------------------------------------------------
set(lidarplugin_sources)
set(lidarplugin_headers) #WIP this looks less effective than classes
set(lidarplugin_classes)
set(lidarplugin_xml)

set(plugin_include_dirs)

# Optional Sources
# Built by default if dependencies are enabled, option available to disable them

# CalibrationFromPoses
cmake_dependent_option(LIDARVIEW_BUILD_CALIBFROMPOSES "Build CalibrationFromPoses" ON "Ceres_FOUND" OFF)
if(LIDARVIEW_BUILD_CALIBFROMPOSES)
  list(APPEND lidarplugin_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CalibrationFromPoses/vtkCalibrationFromPoses.cxx
    ) 
  list(APPEND lidarplugin_xml
    xml/CalibrationFromPoses.xml
    )
    
  list(APPEND plugin_include_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CalibrationFromPoses ) #wip this is wrongly placed

  list(APPEND lidarplugin_classes
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TemporalTransformsRemapper/vtkTemporalTransformsRemapper #wipwip
    )
    
  list(APPEND lidarplugin_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Geometric/vtkGeometricCalibration.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Camera/CameraCalibration.cxx
    )
endif()

# OpenCV extensions
cmake_dependent_option(LIDARVIEW_BUILD_OPENCVEXTENSIONS "Build OpenCV extensions" ON "OpenCV_FOUND" OFF)
if (LIDARVIEW_BUILD_OPENCVEXTENSIONS)
  list(APPEND lidarplugin_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/IO/Camera/vtkPCAPImageReader.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/IO/Camera/vtkOpenCVVideoReader.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/OpenCVImageMapper/vtkOpenCVImageMapper.cxx
    )
  list(APPEND lidarplugin_xml
    xml/PCAPImageReader.xml
    xml/OpenCVImageMapper.xml
    )
  list(APPEND lidarplugin_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkOpenCVConversions.cxx
    )
endif()

# CloudTools
cmake_dependent_option(LIDARVIEW_BUILD_CLOUDTOOLS "Build CloudTools" ON "nanoflann_FOUND" OFF)
if (LIDARVIEW_BUILD_CLOUDTOOLS)
  list(APPEND lidarplugin_sources
       ${CMAKE_CURRENT_SOURCE_DIR}/Filter/SeparateCloudKnn/vtkSeparateCloudKnn.cxx
       ${CMAKE_CURRENT_SOURCE_DIR}/Filter/DBSCANClustering/vtkDBSCANClustering.cxx
       )
  list(APPEND lidarplugin_xml
       xml/SeparateCloudKnn.xml
       xml/DBSCANClustering.xml
       )
  list(APPEND lidarplugin_sources
       ${CMAKE_CURRENT_SOURCE_DIR}/Common/SegmentedCloudTransformations.cxx
       )
endif()

# RegistrationTools
cmake_dependent_option(LIDARVIEW_BUILD_REGTOOLS "Build RegistrationTools" ON "PCL_FOUND;Ceres_FOUND" OFF)
if (LIDARVIEW_BUILD_REGTOOLS)
  list(APPEND lidarplugin_sources
      ${CMAKE_CURRENT_SOURCE_DIR}/Common/RegistrationTools.cxx )
endif()

# MIDHOG Filter extensions
cmake_dependent_option(LIDARVIEW_BUILD_MIDHOG "Build MIDHOG Filter" ON "PCL_FOUND;Ceres_FOUND;OpenCV_FOUND" OFF)
if (LIDARVIEW_BUILD_MIDHOG)
  list(APPEND lidarplugin_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/CameraLidar/MIDHOG.cxx )
endif()

# PCL Miscs
cmake_dependent_option(LIDARVIEW_BUILD_PCLMISCS "Build PCL Miscs" ON "PCL_FOUND" OFF)
if (LIDARVIEW_BUILD_PCLMISCS)
  list(APPEND lidarplugin_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Filter/PCLRansacModel/vtkPCLRansacModel.cxx )
  list(APPEND lidarplugin_xml
    xml/PCLRansacModel.xml
    )

  list(APPEND lidarplugin_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkPCLConversions.cxx
    )
endif()

# Old Slam sources
# ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization/KalmanFilter.cxx
# ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization/MotionModel/MotionModel.cxx
# ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization/TrajectoryReoptimization.cxx
# ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization
# ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization/MotionModel

# Should dissociate back into wrapped / non-wrapped lists
# LidarPlugin sources 
list(APPEND lidarplugin_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/${interpolator_pach_until_vtk_update}
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Network/NetworkPacket.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Network/vtkPacketFileWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Network/vvPacketSender.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkEigenTools.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/CameraProjection.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Camera/CameraModel.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkConversions.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Temporal/vtkTimeCalibration.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Geometric/vtkCarGeometricCalibration.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkPipelineTools.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkHelper.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/LVTime.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/CrashAnalysing.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/PacketReceiver.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/PacketFileWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/PacketConsumer.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common/NMEAParser.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common/GPSProjectionUtils.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/CategoriesConfig.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/MotionDetector/vtkSphericalMap.cxx
  )

# LidarPlugin xml files that describe the proxy of the wrap sources in paraview
list(APPEND lidarplugin_xml
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

# LidarPlugin Header directories, used to relax inclusion expliciteness
list(APPEND plugin_include_dirs
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Common
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/ML
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Network
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Camera
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Geometric
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/Temporal
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Calib/CameraLidar
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/NanoflannAdaptor
  ${CMAKE_CURRENT_SOURCE_DIR}/IO
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Camera
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Applanix
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/ArduPilotDataFlashLogReader/
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common
  #${CMAKE_CURRENT_SOURCE_DIR}/Filter/CalibrationFromPoses #wipwip this is wrongly placed
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
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrajectoryReoptimization
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/Grid
  )

# LidarPlugin Wrapped Headers / Classes
# Only add to `lidarplugin_classes` or `lidarplugin_headers` if you want to have the wrapping
# WIP Wrapping the whole plugin requires a large filestructure rework and selective classes picking

#list(APPEND lidarplugin_headers
#  ${CMAKE_CURRENT_SOURCE_DIR}/Source/Grid/vtkGridSource.h
#  )

set(lidarplugin_classes)
list(APPEND lidarplugin_classes
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/Grid/vtkGridSource
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Applanix/vtkApplanixPositionReader
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/ArduPilotDataFlashLogReader/vtkArduPilotDataFlashLogReader
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common/vtkPositionOrientationPacketInterpreter
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common/vtkPositionOrientationPacketReader
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/GPS-IMU/Common/vtkPositionOrientationStream
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkInterpreter
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/vtkLidarStream
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/vtkLidarPacketInterpreter
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/Lidar/Common/vtkLidarReader
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkStream
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkTemporalTransformsReader
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkTemporalTransformsWriter
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkLASFileWriter
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/LASFileWriter
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkPCDWriter
  ${CMAKE_CURRENT_SOURCE_DIR}/IO/vtkBoundingBoxReader
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TrailingFrame/vtkTrailingFrame
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/MotionDetector/vtkMotionDetector
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/BirdEyeViewSnap/vtkBirdEyeViewSnap
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CameraProjector/vtkCameraProjector
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/CameraMapper/vtkCameraMapper
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/PlaneProjector/vtkPlaneProjector
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/LidarRawSignalImage/vtkLidarRawSignalImage
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/PointCloudLinearProjector/vtkPointCloudLinearProjector
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/LaplacianInfilling/vtkLaplacianInfilling
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/MLSPosesSmoothing/vtkMLSPosesSmoothing
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/ProcessingSample/vtkProcessingSample
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/Ransac/vtkRansacPlaneModel
  ${CMAKE_CURRENT_SOURCE_DIR}/Filter/TemporalTransformsApplier/vtkTemporalTransformsApplier
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkCustomTransformInterpolator
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/vtkTemporalTransforms
  ${CMAKE_CURRENT_SOURCE_DIR}/Common/Network/vtkPacketFileReader
  )

# WIP should be wrapped ?
#list(APPEND lidarplugin_templates
#  ${CMAKE_CURRENT_SOURCE_DIR}/Common/BoundingBox
#  ${CMAKE_CURRENT_SOURCE_DIR}/Common/DBSCAN/DBSCAN
#  )

# Create Module
vtk_module_add_module(LidarCore
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading                
  SOURCES ${lidarplugin_sources} # Will not trigger a PV wrapping
  CLASSES ${lidarplugin_classes} # Those will be PV ClientServer wrapped #shorcut for sources x.cxx x.h  
  HEADERS ${lidarplugin_headers} # Those will be PV ClientServer wrapped
  #TEMPLATES ${lidarplugin_templates} # Those will be PV ClientServer wrapped
  #PRIVATE_* variants exists and will not be wrapped, CLASSES are always PRIVATE/NOT-WRAPPED
)
paraview_add_server_manager_xmls(
  MODULE LidarCore
  XMLS  ${lidarplugin_xml}
)
#WIP Following line could be removed if explicit path are used in every header file, paraview style
target_include_directories(LidarCore PUBLIC ${plugin_include_dirs} ${CMAKE_CURRENT_SOURCE_DIR}/vtkPatch ) #Relax include path expliciteness, does not help wrapping
# Private Dependencies
target_include_directories(LidarCore ${SYSTEM_OPTION} PUBLIC ${LIDARPLUGIN_DEPS_INCLUDE_DIRS}) #Does not make wrapping work
target_link_libraries     (LidarCore PUBLIC ${LIDARPLUGIN_DEPS})

# fpos_t Consistency, see VTK, vtkjsoncpp #WIP how to import VTK variable
target_compile_definitions(LidarCore
  PUBLIC
    _LARGEFILE_SOURCE
    _LARGEFILE64_SOURCE
    _LARGE_FILES
    _FILE_OFFSET_BITS=64)

#-----------------------------------------------------------------------------
# The Python Bindings are kept for LEGACY REASON  and is pending for removal
# Simply use "import LidarPlugin.LidarCore" and you have acces to the wrapped c++ code
# DO NOT FOR ANY REASON USE the method GetClientSideObject() to have acces to the vtkObject directly.
#-----------------------------------------------------------------------------
# Python Wrapping
if(NOT LV_INSTALL_PYTHON_MODULES_DIR)
  message(FATAL_ERROR "LV_INSTALL_PYTHON_MODULES_DIR not set")
endif()

# Use import ${LV_LIDARPLUGIN_PYTHON_PKG}.LidarCore
set(LV_LIDARPLUGIN_PYTHON_PKG "LidarPlugin")
vtk_module_wrap_python(
  MODULES         LidarCore # Target will be LidarCorePython
  BUILD_STATIC    OFF
  INSTALL_HEADERS OFF
  PYTHON_PACKAGE  "${LV_LIDARPLUGIN_PYTHON_PKG}" # This is the name of the python module to import
  MODULE_DESTINATION  "${LV_INSTALL_PYTHON_MODULES_DIR}" #"." #does not really like #without CMAKE_INSTALL_PREFIX
  LIBRARY_DESTINATION "${LV_INSTALL_LIBRARY_DIR}"
  DEPENDS         VTK::vtkpythonmodules
)

# WIP could maybe simplify import subpackage with this ?
#file(WRITE "${CMAKE_PREFIX_PATH}/${LV_LIDARPLUGIN_PYTHON_PKG}/${LV_LIDARPLUGIN_PYTHON_PKG}/__init__.py" "some code that does the import *")
#see https://stackoverflow.com/questions/3365740/how-to-import-all-submodules 

#[[
# Install Python Module wrapped manually #Could be used to alleviate too long of a path?
add_custom_command(
  TARGET LidarCorePython POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/${LV_LIDARPLUGIN_PYTHON_PKG}" "${CMAKE_PREFIX_PATH}/${LV_INSTALL_PYTHON_MODULES_DIR}"
  COMMENT "Install LidarCorePython"
  USES_TERMINAL
)
]]

#-----------------------------------------------------------------------------
# Build PacketFileSender target which enable to playback a recorded pcap on the network
#-----------------------------------------------------------------------------
add_executable(PacketFileSender StandAloneTools/PacketFileSender.cxx)
target_link_libraries(PacketFileSender LINK_PUBLIC LidarCore)

#-----------------------------------------------------------------------------
# Build PCAPSeparator target which enable to split a recorded pcap on two
#-----------------------------------------------------------------------------
add_executable(PCAPSeparator StandAloneTools/PCAPSeparator.cxx)
target_link_libraries(PCAPSeparator LINK_PUBLIC LidarCore)

#-----------------------------------------------------------------------------
# Build PCAPTester target which enable ???, has been disabled for some time now
#-----------------------------------------------------------------------------
#add_executable(PCAPTester StandAloneTools/PCAPTester.cxx)
#target_link_libraries(PCAPTester LINK_PUBLIC LidarCore)

#-----------------------------------------------------------------------------
# Build StandAlone targets which provide different thirparty tools
#-----------------------------------------------------------------------------
# Tools for point cloud/image segmentation, IO/FramesSeriesUtils.cxx
# Code has hardly been maintained, This is kept for future reference only
# A rewrite in Python would make those tools easier to use
#[[
# ImageDetections
cmake_dependent_option(LIDARVIEW_BUILD_IMGDETECTIONS "Build ImageDetections" ON "PCL_FOUND;OpenCV_FOUND" OFF)
if (LIDARVIEW_BUILD_IMGDETECTIONS)
  add_executable(BBoxFromImagesDetections StandAloneTools/BBoxFromImagesDetections.cxx)
  target_link_libraries(BBoxFromImagesDetections LINK_PUBLIC LidarCore)

  add_executable(LabelCloudFromImagesDetections StandAloneTools/LabelCloudFromImagesDetections.cxx)
  target_link_libraries(LabelCloudFromImagesDetections LINK_PUBLIC LidarCore)

endif()

# PostProcessLabelCloud
cmake_dependent_option(LIDARVIEW_BUILD_POSTPROCLABELCLOUD "Build PostProcessLabelCloud" ON "PCL_FOUND;OpenCV_FOUND;nanoflann_FOUND" OFF)
if (LIDARVIEW_BUILD_POSTPROCLABELCLOUD)
  add_executable(PostProcessLabelCloud StandAloneTools/PostProcessLabelCloud.cxx)
  target_link_libraries(PostProcessLabelCloud LINK_PUBLIC LidarCore)

endif()
]]
#-----------------------------------------------------------------------------
# Option to build some tests
#-----------------------------------------------------------------------------
option(BUILD_TESTING "Build some unitary test" OFF)
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()

#-----------------------------------------------------------------------------
# Install targets and ressources
#-----------------------------------------------------------------------------
# The following executables need to land in `/install` to end in the package
set(executables_to_install
  PacketFileSender
  PCAPSeparator # does not end in the package
  #PCAPTester # been disabled for some time now
  )
install(TARGETS ${executables_to_install}
        RUNTIME DESTINATION "${LV_INSTALL_RUNTIME_DIR}"
        LIBRARY DESTINATION "${LV_INSTALL_LIBRARY_DIR}"
)

#WIPWIP DISABLED FOR LATER macosx install mainly
#[[
set(libraries_to_install
  #LidarCore # ??
  #Other for unix ?
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

# WIP IS THIS STILL A THING in PV59 ?
# This is `normal` Superbuild operation, on APPLE the Superbuild's 
# bundling script will only pick from LV_INSTALL_LIBRARY_DIR,
# external dependencies have to be moved manually from their
# custom install directories to the MacOS hierarchy
# It also enable us to filter more finely what ends up in the bundle
if (APPLE)

  # install libraries
  install(TARGETS ${libraries_to_install}
          RUNTIME DESTINATION ${LV_INSTALL_RUNTIME_DIR}
          LIBRARY DESTINATION ${LV_INSTALL_LIBRARY_DIR}
  )

  # install some dependencies for packaging
  install(FILES ${PCAP_LIBRARY} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  install(FILES ${liblas_LIBRARY} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  install(FILES ${Boost_LIBRARIES} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  FILE(GLOB flann_libraries ${CMAKE_INSTALL_PREFIX}/lib/libflann*)
  FILE(GLOB pcl_libraries ${CMAKE_INSTALL_PREFIX}/lib/libpcl*)
  install(FILES ${flann_libraries} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  install(FILES ${pcl_libraries} DESTINATION ${LV_INSTALL_LIBRARY_DIR})

  # install paraview python modules
  install(DIRECTORY ${ParaView_DIR}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages/ DESTINATION ${LV_INSTALL_LIBRARY_DIR}/../Python)
  file(GLOB python_dylib ${ParaView_DIR}/lib/*Python*.dylib)
  install(FILES ${python_dylib} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  file(GLOB python_so ${ParaView_DIR}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages/vtkmodules/*Python.so)
  install(FILES ${python_so} DESTINATION ${LV_INSTALL_LIBRARY_DIR}/../Python/vtkmodules)

else()
  # WIP THIS IS REALLY WEIRD, but PV5.9 now PV install itself
  # TODO Address this undesired patch linux and windows ??
  # install paraview libs in LV_INSTALL_LIBRARY_DIR
  # needed to run LidarView from install path
  if(UNIX)
    file(GLOB paraview_libs ${ParaView_DIR}/lib/lib*.so*)
    install(FILES ${paraview_libs} DESTINATION ${LV_INSTALL_LIBRARY_DIR})
  endif()

endif()
]]
