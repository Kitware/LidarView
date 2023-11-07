set(classes
  lqAboutDialogReaction
  lqChangeInterfaceReaction
  lqCommandLineOptionsBehavior
  lqInterfaceControlsToolbar
  lqLidarViewManager
  lqMenuSaveAsReaction
  lqOpenPcapReaction
  lqOpenRecentFilesReaction
  lqOpenSensorReaction
  lqStatusBar
  lqUpdateCalibrationReaction
)

set(ui_files
  Resources/UI/lqInterfaceControlsToolbar.ui
)

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

vtk_module_add_module(LidarViewApp::lvApplicationComponents
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${ui_files})
vtk_module_link(LidarViewApp::lvApplicationComponents
  PRIVATE
    Qt5::Core
    Qt5::Widgets)
vtk_module_definitions(LidarViewApp::lvApplicationComponents PRIVATE QT_NO_KEYWORDS)

include(FileModuleInstaller)

# Install interface mode json configuration file in the share folder,
# in the build directory and in the install location.
file_module_install(
  NAME interface_config
  FILES
    Resources/interface_modes_config.json
  INSTALL_DIRECTORY "share"
)
