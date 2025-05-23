# Sanitize checks
if(NOT SOFTWARE_NAME OR NOT SOFTWARE_VENDOR)
  message(FATAL_ERROR "SOFTWARE_NAME or SOFTWARE_VENDOR branding not set")
endif()
if(NOT LIDARVIEW_VERSION_FULL)
  message(FATAL_ERROR "LIDARVIEW_VERSION_ variables not set")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (APPLE)
  # Enable high resolution when using Qt5
  set(high_resolution_capable "true")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MacOSXBundleInfo.plist.in"
    @ONLY)
  unset(high_resolution_capable)
endif ()

set(sources
  LidarViewMainWindow.h
  LidarViewMainWindow.cxx
  LidarViewMainWindow.ui
)

set(resource_files
  Resources/lvResources.qrc
)

set(plugins_targets
  ParaView::paraview_plugins
  LidarView::lidarview_plugins
)

if (LIDARVIEW_USE_LIDARSLAM_PLUGIN)
  list(APPEND plugins_targets LidarSlam::paraview_wrapping)
endif ()

paraview_client_add(
  NAME              ${SOFTWARE_NAME}
  NAMESPACE         "${SOFTWARE_NAME}"
  EXPORT            "LidarViewClient"
  VERSION           "${LIDARVIEW_VERSION_MAJOR}.${LIDARVIEW_VERSION_MINOR}.${LIDARVIEW_VERSION_PATCH}"
  APPLICATION_NAME  "${SOFTWARE_NAME}"
  ORGANIZATION      "${SOFTWARE_VENDOR}"
  TITLE             "${SOFTWARE_NAME} ${LIDARVIEW_VERSION_FULL}"
  SPLASH_IMAGE      "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/Splash.jpg"
  BUNDLE_ICON       "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/logo.icns"
  BUNDLE_PLIST      "${CMAKE_CURRENT_BINARY_DIR}/MacOSXBundleInfo.plist.in"
  APPLICATION_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/logo.ico"

  PLUGINS_TARGETS   ${plugins_targets}
  REQUIRED_PLUGINS  LidarCorePlugin

  MAIN_WINDOW_CLASS LidarViewMainWindow
  MAIN_WINDOW_INCLUDE LidarViewMainWindow.h
  SOURCES ${sources} ${resource_files}
  APPLICATION_XMLS
    ${CMAKE_CURRENT_SOURCE_DIR}/lvSources.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/lvFilters.xml

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
    LidarViewApp::lvApplicationComponents # Specific LidarView code base
)
