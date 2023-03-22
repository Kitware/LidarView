set(classes
  vtkLidarPacketInterpreter
  vtkLidarReader
  vtkLidarStream)

vtk_module_add_module(LidarView::IOLidar
  FORCE_STATIC
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/LidarPacketInterpreter.xml
    Resources/LidarReader.xml
    Resources/LidarStream.xml)
