set(classes
  lqLidarViewManager
  lqOpenPcapReaction
  lqOpenRecentFilesReaction
  lqOpenSensorReaction
  lqUpdateCalibrationReaction
)

set(headers
  lqPythonQtLidarView.h # Pending delete
)

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)

vtk_module_add_module(LidarViewApp::lvApplicationComponents
  CLASSES ${classes}
  HEADERS ${headers})
vtk_module_link(LidarViewApp::lvApplicationComponents
  PRIVATE
    Qt5::Core
    Qt5::Widgets)
vtk_module_definitions(LidarViewApp::lvApplicationComponents PRIVATE QT_NO_KEYWORDS)
