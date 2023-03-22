set(classes
  LASFileWriter
  vtkLASFileWriter
  vtkPCDWriter)

vtk_module_add_module(LidarView::IOGeneral
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/LASFileWriter.xml
    Resources/PCDWriter.xml)
