set(classes
  vtkLidarPacketInterpreter
  vtkLidarReader
  vtkLidarStream)

vtk_module_add_module(LidarView::IOLidar
  CLASSES ${classes})
