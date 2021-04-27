#Sanitize checks
if(NOT LV_INSTALL_LIBRARY_DIR )
  message(FATAL_ERROR "LV_INSTALL_LIBRARY_DIR not set")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(lqApplicationComponents
  ctk/ctkCoreExport.h
  ctk/ctkDoubleRangeSlider.cpp
  ctk/ctkDoubleRangeSlider.h
  ctk/ctkPimpl.h
  ctk/ctkRangeSlider.cpp
  ctk/ctkRangeSlider.h
  ctk/ctkValueProxy.cpp
  ctk/ctkValueProxy.h
  ctk/ctkWidgetsExport.h
  lqCameraParallelProjectionReaction.cxx
  lqCameraParallelProjectionReaction.h
  lqColorToolbar.cxx
  lqColorToolbar.h
  lqColorToolbar.ui
  lqDockableSpreadSheetReaction.h
  lqDockableSpreadSheetReaction.cxx
  lqPlayerControlsToolbar.cxx
  lqPlayerControlsToolbar.h
  lqPlayerControlsToolbar.ui
  lqPlayerControlsController.cxx
  lqPlayerControlsController.h
  lqStreamRecordDialog.cxx
  lqStreamRecordDialog.h
  lqStreamRecordReaction.cxx
  lqStreamRecordReaction.h
  lqViewFrameActions.cxx
  lqViewFrameActions.h
  lqRulerReaction.h
  lqRulerReaction.cxx
  lqLiveSourceScalarColoringBehavior.cxx
  lqLiveSourceScalarColoringBehavior.h
  SaveAndLoadLidarState/lqSaveLidarStateReaction.cxx
  SaveAndLoadLidarState/lqSaveLidarStateReaction.h
  SaveAndLoadLidarState/lqLoadLidarStateReaction.cxx
  SaveAndLoadLidarState/lqLoadLidarStateReaction.h
  SaveAndLoadLidarState/lqLidarStateDialog.cxx
  SaveAndLoadLidarState/lqLidarStateDialog.h
  LidarCameraToolbar/lqLidarCameraReaction.cxx
  LidarCameraToolbar/lqLidarCameraReaction.h
  LidarCameraToolbar/lqLidarCameraToolbar.cxx
  LidarCameraToolbar/lqLidarCameraToolbar.h
  LidarCameraToolbar/lqLidarCameraToolbar.ui
  lqEnableAdvancedArraysReaction.cxx
  lqEnableAdvancedArraysReaction.h
  SensorWidget/lqSensorListWidget.cxx
  SensorWidget/lqSensorListWidget.h
  SensorWidget/lqSensorWidget.cxx
  SensorWidget/lqSensorWidget.h
  SensorWidget/lqSensorWidget.ui
  SensorWidget/lqSensorStreamWidget.cxx
  SensorWidget/lqSensorStreamWidget.h
  SensorWidget/lqSensorReaderWidget.cxx
  SensorWidget/lqSensorReaderWidget.h
  lqResources.qrc
  )
target_include_directories(lqApplicationComponents PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/ctk/
  ${CMAKE_CURRENT_SOURCE_DIR}/SaveAndLoadLidarState/
  ${CMAKE_CURRENT_SOURCE_DIR}/LidarCameraToolbar/
  ${CMAKE_CURRENT_SOURCE_DIR}/SensorWidget/
  ${CMAKE_CURRENT_BINARY_DIR}
  )
target_sources(lqApplicationComponents PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/lqHelper.cxx
  )
target_link_libraries(lqApplicationComponents
    PUBLIC
    pqApplicationComponents
    LidarPlugin)
target_compile_definitions(lqApplicationComponents PUBLIC Q_OS_SYMBIAN)
target_compile_definitions(lqApplicationComponents PRIVATE CTKCore_EXPORTS CTKWidgets_EXPORTS)
include(GenerateExportHeader)
generate_export_header(lqApplicationComponents)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lqapplicationcomponents_export.h DESTINATION ${LV_INSTALL_LIBRARY_DIR})
# install libraries
if (APPLE)
  install(TARGETS lqApplicationComponents
          RUNTIME DESTINATION ${LV_INSTALL_LIBRARY_DIR}
          LIBRARY DESTINATION ${LV_INSTALL_LIBRARY_DIR}
          COMPONENT Runtime)
else()
  install(TARGETS lqApplicationComponents
          RUNTIME DESTINATION "bin"
          LIBRARY DESTINATION ${LV_INSTALL_LIBRARY_DIR}
          COMPONENT Runtime)
endif()
