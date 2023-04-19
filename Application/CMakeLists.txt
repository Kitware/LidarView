# Sanitize checks
if(NOT SOFTWARE_NAME OR NOT SOFTWARE_VENDOR)
  message(FATAL_ERROR "SOFTWARE_NAME or SOFTWARE_VENDOR branding not set")
endif()
if(NOT LV_VERSION_FULL)
  message(FATAL_ERROR "LV_VERSION_ variables not set")
endif()

add_subdirectory(Ui/)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(source_files
  vvMainWindow.h
  vvMainWindow.ui
  vvResources.qrc
  vvMainWindow.cxx
  vvMainWindow.h
)

# Please make sure to adapt the AboutDialog text in the followin file
#  Lidar\python\lidarview\aboutDialog.py
# You also need to change:
#   - bottom_logo.png (bottom logo)

set(plugins_targets
  ParaView::paraview_plugins
  LidarView::lidarview_plugins
)

if (PARAVIEW_PLUGIN_ENABLE_LidarSlam)
  list(APPEND plugins_targets LidarSlam::paraview_wrapping)
endif ()

paraview_client_add(
  NAME              LidarView
  NAMESPACE         "LidarView"
  EXPORT            "LidarViewClient"
  VERSION           ${LV_VERSION_FULL}
  APPLICATION_NAME  "LidarView"
  ORGANIZATION      "${SOFTWARE_VENDOR}"
  TITLE             "${SOFTWARE_NAME} ${LV_VERSION_FULL} ${LV_BUILD_PLATFORM}"
  SPLASH_IMAGE      "${CMAKE_CURRENT_SOURCE_DIR}/SoftwareInformation/Splash.jpg"
  BUNDLE_ICON       "${CMAKE_CURRENT_SOURCE_DIR}/logo.icns"
  APPLICATION_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/SoftwareInformation/logo.ico"

  PLUGINS_TARGETS   ${plugins_targets}
  REQUIRED_PLUGINS  PythonQtPlugin LidarCorePlugin

  MAIN_WINDOW_CLASS vvMainWindow
  MAIN_WINDOW_INCLUDE vvMainWindow.h
  SOURCES ${source_files}
  APPLICATION_XMLS
    ${CMAKE_CURRENT_SOURCE_DIR}/lqSources.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/lqFilters.xml

  RUNTIME_DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY_DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

target_include_directories(${SOFTWARE_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${SOFTWARE_NAME}
  PRIVATE
    LidarView::lqApplicationComponents # Common LVCore code base
    ApplicationUi
)

# Bundle Icon on Apple
if(APPLE)
  get_target_property(MACOSX_BUNDLE_ICON_FILE ${SOFTWARE_NAME} MACOSX_BUNDLE_ICON_FILE)
  install(FILES ${MACOSX_BUNDLE_ICON_FILE} DESTINATION "${LV_INSTALL_RESOURCE_DIR}") # to install tree
endif()
