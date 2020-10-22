set(server_manager_xml
  Kitti/data/LidarKITTIDataSetReader.xml
  Kitti/data/LidarKITTIDataSetWriter.xml
  Kitti/labels/KITTIObjectLabelsReader.xml

)

set(server_manager_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/Kitti/data/vtkLidarKITTIDataSetReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Kitti/data/vtkLidarKITTIDataSetWriter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/Kitti/labels/vtkKITTIObjectLabelsReader.cxx
)

set(server_sources
  )

ADD_PARAVIEW_PLUGIN(DatasetIOPlugin "1.0"
  SERVER_MANAGER_XML ${server_manager_xml}
  SERVER_MANAGER_SOURCES ${server_manager_sources}
  SERVER_SOURCES ${server_sources})

target_link_libraries(DatasetIOPlugin PUBLIC LidarPlugin)
target_include_directories(DatasetIOPlugin
  PUBLIC
  Kitti/
  Kitti/data
  Kitti/labels
)

# install libraries
if (APPLE)
  install(TARGETS DatasetIOPlugin
          RUNTIME DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/MacOS/plugins
          LIBRARY DESTINATION ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/MacOS/plugins
          COMPONENT Runtime)

else()
  install(TARGETS DatasetIOPlugin
          RUNTIME DESTINATION ${VV_INSTALL_LIBRARY_DIR}/plugins
          LIBRARY DESTINATION ${VV_INSTALL_LIBRARY_DIR}/plugins
          COMPONENT Runtime)
endif()

