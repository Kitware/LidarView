set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(lqApplicationComponents
  ctk/ctkCoreExport.h
  ctk/ctkDoubleRangeSlider.cpp
  ctk/ctkDoubleRangeSlider.h
  ctk/ctkPimpl.h
  ctk/ctkRangeSlider.cpp
  ctk/ctkRangeSlider.h
  ctk/ctkValueProxy.cpp
  ctk/ctkValueProxy.h
  ctk/ctkWidgetsExport.h
  vvColorToolbar.cxx
  vvColorToolbar.h
  vvColorToolbar.ui
  vvPlayerControlsToolbar.cxx
  vvPlayerControlsToolbar.h
  vvPlayerControlsToolbar.ui
  vvPlayerControlsController.cxx
  vvPlayerControlsController.h
  )
target_include_directories(lqApplicationComponents PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/ctk/
  ${CMAKE_CURRENT_BINARY_DIR}
  )
target_link_libraries(lqApplicationComponents PUBLIC pqApplicationComponents)
target_compile_definitions(lqApplicationComponents PUBLIC Q_OS_SYMBIAN)
target_compile_definitions(lqApplicationComponents PRIVATE CTKCore_EXPORTS CTKWidgets_EXPORTS)
include(GenerateExportHeader)
generate_export_header(lqApplicationComponents)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lqapplicationcomponents_export.h DESTINATION ${VV_INSTALL_LIBRARY_DIR})
# install libraries
if (APPLE)
  install(TARGETS lqApplicationComponents
          RUNTIME DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Libraries
          LIBRARY DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Libraries
          COMPONENT Runtime)
else()
  install(TARGETS lqApplicationComponents
          RUNTIME DESTINATION ${VV_INSTALL_RUNTIME_DIR}
          LIBRARY DESTINATION ${VV_INSTALL_LIBRARY_DIR}
          COMPONENT Runtime)
endif()
