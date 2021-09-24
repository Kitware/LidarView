#Sanitize checks
if(NOT LV_INSTALL_LIBRARY_DIR )
  message(FATAL_ERROR "LV_INSTALL_LIBRARY_DIR not set")
endif()

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

set(server_sources)

# Create Plugin
paraview_add_plugin(DatasetIOPlugin
  VERSION "1.0"
  SERVER_MANAGER_XML ${server_manager_xml}
  SOURCES ${server_manager_sources} ${server_sources}
)

target_link_libraries(DatasetIOPlugin PUBLIC LidarCore)
target_include_directories(DatasetIOPlugin
  PUBLIC
  Kitti/
  Kitti/data
  Kitti/labels
)
