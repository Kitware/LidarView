set(interfaces)
set(sources
  lqCustomDockWidget.cxx
  lqCustomDockWidget.h
  lqDockToolBar.cxx
  lqDockToolBar.h
)
set(ui_files
  Resources/UI/CustomDockWidget.ui
)
set(qrc_files
  Resources/CustomDockResources.qrc
)

# ParaView cmake macro to add a Qt dock wiget in an external plugin.
# It generate files that are added in `dock_interfaces` and `dock_sources` variables.
paraview_plugin_add_dock_window(
  CLASS_NAME lqCustomDockWidget
  DOCK_AREA Right
  INTERFACES dock_interfaces
  SOURCES dock_sources)
list(APPEND interfaces
  ${dock_interfaces})
list(APPEND sources
  ${dock_sources})

# ParaView cmake macro to add a Qt toolbar in an external plugin.
# It generate files that are added in `toolbar_interfaces` and `toolbar_sources` variables.
paraview_plugin_add_toolbar(
  CLASS_NAME lqDockToolBar
  INTERFACES toolbar_interfaces
  SOURCES toolbar_sources)
list(APPEND interfaces
  ${toolbar_interfaces})
list(APPEND sources
  ${toolbar_sources})

# If UI files are not in the current source or binary directory, they may be
# searched for by adding its directory to `CMAKE_AUTOUIC_SEARCH_PATHS`.
list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

paraview_add_plugin(CustomDockWidget
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  UI_FILES ${ui_files}
  UI_RESOURCES ${qrc_files}
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}

  MODULES
    CustomDockWidget::DummyExternalAPI
  MODULE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/DummyExternalAPI/vtk.module"
)

target_link_libraries(CustomDockWidget
  PRIVATE
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")
