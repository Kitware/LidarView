set(classes
  lqCameraParallelProjectionReaction
  lqColorToolbar
  lqDockableSpreadSheetReaction
  lqEnableAdvancedArraysReaction
  lqLidarCameraReaction
  lqLidarCameraToolbar
  lqLidarCoreManager
  lqLidarStreamColorByInitBehavior
  lqLiveSourceScalarColoringBehavior
  lqLoadLidarStateReaction
  lqPlayerControlsController # Should be in Components but has a dependency on lqSensorListWidget
  lqPlayerControlsToolbar
  lqRulerReaction
  lqSaveLASReaction
  lqSaveLidarFrameReaction
  lqSaveLidarStateReaction
  lqSensorListWidget
  lqSensorReaderWidget
  lqSensorStreamWidget
  lqSensorWidget
  lqStreamRecordReaction
  lqViewFrameActions)

set(headers
  lqPythonQtDecorators.h # Should have it's own vtk module, but currently circular dependency
)

set(ui_files
  Resources/UI/lqColorToolbar.ui
  Resources/UI/lqLidarCameraToolbar.ui
  Resources/UI/lqPlayerControlsToolbar.ui
  Resources/UI/lqSensorWidget.ui)

set(resource_files
  Resources/lqResources.qrc)

source_group("Resources" FILES
  ${ui_files}
  ${resource_files})

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

vtk_module_add_module(LidarView::lqApplicationComponents
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${ui_files}
          ${resource_files})
vtk_module_link(LidarView::lqApplicationComponents
  PRIVATE
    Qt5::Core
    Qt5::Widgets)
vtk_module_definitions(LidarView::lqApplicationComponents PRIVATE QT_NO_KEYWORDS)
