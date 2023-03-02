# Sanitize checks
if(NOT LV_INSTALL_LIBRARY_DIR )
  message(FATAL_ERROR "LV_INSTALL_LIBRARY_DIR not set")
endif()
if(NOT SOFTWARE_NAME OR NOT SOFTWARE_VENDOR)
  message(FATAL_ERROR "SOFTWARE_NAME or SOFTWARE_VENDOR branding not set")
endif()
if(NOT LV_VERSION_FULL)
  message(FATAL_ERROR "LV_VERSION_ variables not set")
endif()
if(NOT LV_BUILD_PLATFORM)
  message(FATAL_ERROR "LV_BUILD_PLATFORM not set")
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

# Please make sure to adapt the AboutDialog text in the followin file
#  Lidar\python\lidarview\aboutDialog.py
# You also need to change:
#   - bottom_logo.png (bottom logo)

paraview_client_add(
  NAME ${SOFTWARE_NAME} #Name of the Target
  NAMESPACE "LidarView"
  VERSION ${LV_VERSION_FULL}
  APPLICATION_NAME "${SOFTWARE_NAME}"
  ORGANIZATION "${SOFTWARE_VENDOR}"
  TITLE "${SOFTWARE_NAME} ${LV_VERSION_FULL} ${LV_BUILD_PLATFORM}"
  SPLASH_IMAGE     "${CMAKE_CURRENT_SOURCE_DIR}/SoftwareInformation/Splash.jpg"
  BUNDLE_ICON      "${CMAKE_CURRENT_SOURCE_DIR}/logo.icns"
  APPLICATION_ICON "${CMAKE_CURRENT_SOURCE_DIR}/SoftwareInformation/logo.ico"
  
  REQUIRED_PLUGINS LidarPlugin VelodynePlugin PythonQtPlugin

  MAIN_WINDOW_CLASS vvMainWindow
  MAIN_WINDOW_INCLUDE vvMainWindow.h
  SOURCES ${source_files}
  APPLICATION_XMLS
    ${CMAKE_CURRENT_SOURCE_DIR}/lqSources.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/lqFilters.xml
  
  RUNTIME_DESTINATION ${LV_INSTALL_RUNTIME_DIR}
  LIBRARY_DESTINATION ${LV_INSTALL_LIBRARY_DIR}
)

target_include_directories(${SOFTWARE_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

# We directly link against the plugin and import them statically.
target_link_libraries(${SOFTWARE_NAME} PUBLIC
  lqApplicationComponents #actually LVCore/ApplicationComponents
  ApplicationUi
  )

# Bundle Icon on Apple
if(APPLE)
  get_target_property(MACOSX_BUNDLE_ICON_FILE ${SOFTWARE_NAME} MACOSX_BUNDLE_ICON_FILE)
  install(FILES ${MACOSX_BUNDLE_ICON_FILE} DESTINATION "${LV_INSTALL_RESOURCE_DIR}") # to install tree
endif()
