set(required_modules)

#[==[.md
## Add xml files and modules

Test vtk modules target existance, if exist: add corresponding xml files and
vtk module target to the list to be built (xml_files and required_modules).

  * `MODULE_TARGET`: (Required) The target checked if it exists
  * `XML_MODULE_FILES`: (Required) Server manager XML files to be added
    for the plugin.
#]==]
macro (lidarcoreplugin_add_module_xml)
  cmake_parse_arguments(_lidarcoreplugin_add_module_xml
    ""
    "MODULE_TARGET"
    "XML_MODULE_FILES"
    ${ARGN})

  if (_lidarcoreplugin_add_module_xml_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for lidarcoreplugin_add_module_xml: "
      "${_lidarcoreplugin_add_module_xml_UNPARSED_ARGUMENTS}")
  endif ()
  if (NOT DEFINED _lidarcoreplugin_add_module_xml_XML_MODULE_FILES OR
    NOT DEFINED _lidarcoreplugin_add_module_xml_MODULE_TARGET)
    message(FATAL_ERROR
      "`MODULE_TARGET` and `XML_MODULE_FILES` arguments are required.")
  endif ()

  if (TARGET ${_lidarcoreplugin_add_module_xml_MODULE_TARGET})
    paraview_add_server_manager_xmls(
      MODULE ${_lidarcoreplugin_add_module_xml_MODULE_TARGET}
      XMLS
        ${_lidarcoreplugin_add_module_xml_XML_MODULE_FILES}
    )
    list(APPEND required_modules ${_lidarcoreplugin_add_module_xml_MODULE_TARGET})
  endif()
endmacro ()


lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::FiltersCalibration
  XML_MODULE_FILES
    Filters/CalibrationFromPoses.xml
    Filters/TemporalTransformsRemapper.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::FiltersCamera
  XML_MODULE_FILES
    Filters/CameraMapper.xml
    Filters/LaplacianInfilling.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::FiltersCameraOpenCV
  XML_MODULE_FILES
    Filters/CameraProjector.xml
    Filters/OpenCVImageMapper.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::FiltersGeneral
  XML_MODULE_FILES
    Filters/ClipPoints.xml
    Filters/RPMText.xml
    Filters/SelectPointsByScalars.xml
)

set(xml_files
  Filters/PlaneProjector.xml
  Filters/RansacPlaneModel.xml
  Filters/EuclideanCompare.xml
  Filters/ComputeVolume.xml
  Filters/ImagesOperations.xml
  Filters/VoxelOverlapFilter.xml
)

if (LIDARVIEW_USE_NANOFLANN)
  list(APPEND xml_files
    Filters/AdaptiveOutlierRemoval.xml
    Filters/ComputeNormalAdaptive.xml)
endif()

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::FiltersProcessing
  XML_MODULE_FILES
    ${xml_files}
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::FiltersSegmentation
  XML_MODULE_FILES
    Filters/DBSCANClustering.xml
    Filters/SeparateCloudKnn.xml
    Filters/ThresholdPoints.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::FiltersSelection
  XML_MODULE_FILES
    Filters/ExtractPointSelection.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::FiltersTemporal
  XML_MODULE_FILES
    Filters/AggregatePointsFromTrajectory.xml
    Filters/AggregatePointsFromTrajectoryOnline.xml
    Filters/AggregatePointsFromTrajectoryOffline.xml
    Filters/AssociateTableToTemporalDataset.xml
    Filters/ClusteringAndTracking.xml
    Filters/MLSPosesSmoothing.xml
    Filters/MotionDetector.xml
    Filters/TemporalTransformsApplier.xml
    Filters/TrailingFrame.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::FiltersVisualization
  XML_MODULE_FILES
    Filters/BirdEyeViewSnap.xml
    Filters/LidarRawSignalImage.xml
    Filters/PointCloudLinearProjector.xml
    Filters/PointCloudPinHoleProjector.xml
    Filters/VoxelGridFilter.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::IOCameraOpenCV
  XML_MODULE_FILES
    IO/OpenCVVideoReader.xml
    IO/PCAPImageReader.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::IOCore
  XML_MODULE_FILES
    IO/TemporalTransformsReader.xml
    IO/TemporalTransformsWriter.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::IOGeneral
  XML_MODULE_FILES
    IO/PCDWriter.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::IOGeolocation
  XML_MODULE_FILES
    IO/ApplanixPositionReader.xml
    IO/ArduPilotDataFlashLogReader.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::IOLidar
  XML_MODULE_FILES
    IO/Interpreters/LidarReader.xml
    IO/Interpreters/LidarStream.xml
    IO/Interpreters/PacketInterpreter.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::IONetwork
  XML_MODULE_FILES
    IO/Interpreters/BasePacketInterpreter.xml
    IO/Interpreters/BaseStream.xml
    IO/Interpreters/PacketReceiver.xml
    IO/UDPPing.xml
    IO/UDPPointSender.xml
    IO/UDPSenderAlgorithm.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::IOPDAL
  XML_MODULE_FILES
    IO/LASWriter.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::IOYaml
  XML_MODULE_FILES
    IO/BoundingBoxReader.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::RemotingServerManager
  XML_MODULE_FILES
    Remoting/InterpretersManager.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::RemotingViews
  XML_MODULE_FILES
    Remoting/LidarGridView.xml
)

lidarcoreplugin_add_module_xml(
  MODULE_TARGET LidarView::Sources
  XML_MODULE_FILES
    Sources/GridSource.xml
)

if (LIDARVIEW_USE_QT)
  set(interfaces)
  set(ui_files
    Resources/UI/lqCalibrationFileWidget.ui)
  set(sources
    lqCalibrationFilePropertyWidget.cxx
    lqCalibrationFilePropertyWidget.h
    lqConditionalDefaultPropertyWidgetDecorator.cxx
    lqConditionalDefaultPropertyWidgetDecorator.h
    lqLidarCorePluginManager.cxx
    lqLidarCorePluginManager.h)

  paraview_plugin_add_property_widget(
    KIND WIDGET
    TYPE "calibration_file_widget"
    CLASS_NAME lqCalibrationFilePropertyWidget
    INTERFACES calibration_file_widget_interfaces
    SOURCES calibration_file_widget_sources)

  paraview_plugin_add_property_widget(
    KIND WIDGET_DECORATOR
    TYPE "ConditionalDefaultValueDecorator"
    CLASS_NAME lqConditionalDefaultPropertyWidgetDecorator
    INTERFACES conditional_default_decorator_interfaces
    SOURCES conditional_default_decorator_sources)

  paraview_plugin_add_auto_start(
    CLASS_NAME lqLidarCorePluginManager
    STARTUP onStartup
    SHUTDOWN onShutdown
    INTERFACES autostart_interface
    SOURCES autostart_sources)

  list(APPEND interfaces
    ${calibration_file_widget_interfaces}
    ${conditional_default_decorator_interfaces}
    ${autostart_interface})
  list(APPEND sources
    ${calibration_file_widget_sources}
    ${conditional_default_decorator_sources}
    ${autostart_sources})

  set(qrc_files
    Resources/lqFilters.qrc
  )

  # As the ui files are not in the current source diectory
  list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")
endif ()

paraview_add_plugin(LidarCorePlugin
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  UI_RESOURCES ${qrc_files}
  UI_INTERFACES ${interfaces}
  UI_FILES ${ui_files}
  SOURCES ${sources}
  MODULES ${required_modules}
)
