set(classes
  lqAboutDialog
)

set(ui_files
  Resources/UI/lqAboutDialog.ui
)

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
  SOURCES ${ui_files}
          ${resource_files})
vtk_module_link(LidarViewApp::lvComponents
  PRIVATE
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core"
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")

target_compile_definitions(lvComponents PRIVATE QT_NO_KEYWORDS)
