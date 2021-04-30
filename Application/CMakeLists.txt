#Sanitize checks
if(NOT LV_INSTALL_LIBRARY_DIR )
  message(FATAL_ERROR "LV_INSTALL_LIBRARY_DIR not set")
endif()
if(NOT SOFTWARE_NAME OR NOT SOFTWARE_VENDOR)
  message(FATAL_ERROR "SOFTWARE_NAME or SOFTWARE_VENDOR branding not set")
endif()
if(NOT LV_VERSION_FULL)
  message(FATAL_ERROR "LV_VERSION_ variables not set")
endif()
if(NOT LV_BUILD_ARCHITECTURE)
  message(FATAL_ERROR "LV_BUILD_ARCHITECTURE not set")
endif()
if(NOT paraview_version)
  message(FATAL_ERROR "paraview_version not set")
endif()

add_subdirectory("Ui/")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set (source_files
  vvMainWindow.h
  vvMainWindow.ui
  vvResources.qrc
  vvMainWindow.cxx
  vvMainWindow.h
  )

# This ensure that build_paraview_client() adds an "forwarding path" to locate
# paraview from installed location.
set (PARAVIEW_INSTALL_LIB_DIR "lib/paraview-${paraview_version}")

# Please make sure to adapt the AboutDialog text in the followin file
#  Lidar\python\lidarview\aboutDialog.py
# You also need to change:
#   - bottom_logo.png (bottom logo)

paraview_client_add(
  NAME lidarview #wip should be lidarview right?
  NAMESPACE "LidarView"
  VERSION ${LV_VERSION_FULL}
  APPLICATION_NAME "${SOFTWARE_NAME}"
  ORGANIZATION "${SOFTWARE_VENDOR}"
  TITLE "${SOFTWARE_NAME} ${LV_VERSION_FULL} ${LV_BUILD_ARCHITECTURE}-bit"
  SPLASH_IMAGE     "${CMAKE_CURRENT_SOURCE_DIR}/SoftwareInformation/Splash.jpg"
  BUNDLE_ICON      "${CMAKE_CURRENT_SOURCE_DIR}/logo.icns"
  APPLICATION_ICON "${CMAKE_CURRENT_SOURCE_DIR}/SoftwareInformation/logo.ico"
  
  #REQUIRED_PLUGINS LidarPlugin #unknown
  PLUGINS_TARGETS   LidarView::LidarPlugin
  
  MAIN_WINDOW_CLASS vvMainWindow
  MAIN_WINDOW_INCLUDE vvMainWindow.h
  SOURCES ${source_files}
  APPLICATION_XMLS
    ${CMAKE_CURRENT_SOURCE_DIR}/vvSources.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/vvFilters.xml
    
  #BUNDLE_DESTINATION #wip useful macosx?
  RUNTIME_DESTINATION ${LV_INSTALL_RUNTIME_DIR}
  LIBRARY_DESTINATION ${LV_INSTALL_LIBRARY_DIR}
)

target_include_directories(${SOFTWARE_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

# We directly link against the plugin and import them statically.
if(NOT PYTHONQTPLUGIN_LIBRARY OR NOT PYTHONQT_LIBRARIES)
  message(FATAL_ERROR "PYTHONQTPLUGIN_LIBRARY OR PYTHONQT_LIBRARIES not set")
endif()
target_link_libraries(${SOFTWARE_NAME}
  PUBLIC
    LidarPlugin
    ApplicationUi
    VelodynePlugin
    ${PYTHONQTPLUGIN_LIBRARY}
    ${PYTHONQT_LIBRARIES}
    lqApplicationComponents)
