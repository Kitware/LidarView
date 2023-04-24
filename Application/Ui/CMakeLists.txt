set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(ApplicationUi
  Widgets/vvCalibrationDialog.cxx
  Widgets/vvCalibrationDialog.h
  Widgets/vvCalibrationStructs.h
  Widgets/vvCalibrationDialog.ui
  Widgets/vvCropReturnsDialog.cxx
  Widgets/vvCropReturnsDialog.h
  Widgets/vvCropReturnsDialog.ui
  Widgets/vvSelectFramesDialog.cxx
  Widgets/vvSelectFramesDialog.h
  Widgets/vvSelectFramesDialog.ui
  Widgets/images/resources.qrc

  lqLidarViewManager.cxx
  lqLidarViewManager.h
  lqOpenPcapReaction.cxx
  lqOpenPcapReaction.h
  lqOpenRecentFilesReaction.cxx
  lqOpenRecentFilesReaction.h
  lqOpenSensorReaction.cxx
  lqOpenSensorReaction.h
  lqUpdateCalibrationReaction.cxx
  lqUpdateCalibrationReaction.h
  lqPythonQtLidarView.h # LV Specific PythonQt Decorators
)

target_link_libraries(ApplicationUi PUBLIC
  ParaView::pqCore
  ParaView::pqWidgets
  ParaView::pqComponents
  ParaView::pqApplicationComponents
  ParaView::pqPython
  ParaView::pvpythonmodules
  ParaView::PythonInitializer
  ParaView::RemotingAnimation
  ParaView::RemotingApplication
  ParaView::RemotingClientServerStream
  ParaView::RemotingCore
  ParaView::RemotingLive
  ParaView::RemotingMisc
  ParaView::RemotingServerManager
  ParaView::RemotingServerManagerPython
  ParaView::RemotingSettings
  ParaView::RemotingViews
  ParaView::RemotingViewsPython

  # Common LVCore code base
  LidarView::lqApplicationComponents
  LidarView::lqCore

  # Required to Wrap additional functions
  PythonQt::PythonQt
)

target_include_directories(ApplicationUi PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Widgets
  ${CMAKE_CURRENT_BINARY_DIR}
  )

target_compile_definitions(ApplicationUi
  PRIVATE
  "LIDARVIEW_BUILD_VELODYNE=$<BOOL:${PARAVIEW_PLUGIN_ENABLE_VelodynePlugin}>"
  "LIDARVIEW_BUILD_HESAI=$<BOOL:${PARAVIEW_PLUGIN_ENABLE_HesaiPlugin}>"
)

include(GenerateExportHeader)
generate_export_header(ApplicationUi)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/applicationui_export.h DESTINATION ${LV_INSTALL_LIBRARY_DIR})

# Install Library needed
install(TARGETS ApplicationUi
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
