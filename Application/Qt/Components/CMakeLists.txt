set(classes
  lqAboutDialog
  lqCalibrationDialog
  lqCropReturnsDialog
  lqSelectFramesDialog
)

set(headers
  lqCalibrationStructs.h
)

set(ui_files
  Resources/UI/lqAboutDialog.ui
  Resources/UI/lqCalibrationDialog.ui
  Resources/UI/lqCropReturnsDialog.ui
  Resources/UI/lqSelectFramesDialog.ui)

set(resource_files
  Resources/lvComponents.qrc
)

source_group("Resources" FILES
  ${ui_files}
  ${resource_files})

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

vtk_module_add_module(LidarViewApp::lvComponents
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${ui_files}
          ${resource_files})
vtk_module_link(LidarViewApp::lvComponents
  PRIVATE
    Qt5::Core
    Qt5::Widgets)
vtk_module_definitions(LidarViewApp::lvComponents
  PRIVATE
    QT_NO_KEYWORDS
    "LIDARVIEW_BUILD_VELODYNE=$<BOOL:${PARAVIEW_PLUGIN_ENABLE_VelodynePlugin}>"
    "LIDARVIEW_BUILD_HESAI=$<BOOL:${PARAVIEW_PLUGIN_ENABLE_HesaiPlugin}>"
)
