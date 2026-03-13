set(xml_files
  KITTIObjectLabelsReader.xml
  LidarKITTIDataSetReader.xml
  LidarKITTIDataSetWriter.xml
)

set(classes
  vtkKITTIObjectLabelsReader
  vtkLidarKITTIDataSetReader
  vtkLidarKITTIDataSetWriter
)

vtk_module_add_module(DatasetIOPlugin::vtkKittiIO
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS ${xml_files})
