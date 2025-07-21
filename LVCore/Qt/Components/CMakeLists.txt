set(classes
  lqChooseLidarDialog
  lqInterpreterWidget
  lqLidarConfigurationDialog
  lqLidarStateDialog
  lqSelectLidarFrameDialog
  lqStreamRecordController
  lqStreamRecordDialog
  lqTimestampLineEdit
)

set(ui_files
  Resources/UI/lqChooseLidarDialog.ui
  Resources/UI/lqLidarConfigurationDialog.ui
  Resources/UI/lqSelectLidarFrameDialog.ui
  Resources/UI/lqStreamRecordDialog.ui
)

source_group("Resources" FILES
  ${ui_files})

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

vtk_module_add_module(LidarView::lqComponents
  CLASSES ${classes}
  SOURCES ${ui_files})
vtk_module_link(LidarView::lqComponents
  PRIVATE
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core"
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")
vtk_module_definitions(LidarView::lqComponents PRIVATE QT_NO_KEYWORDS)
