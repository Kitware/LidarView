set(classes
  LASFileWriter
  vtkLASFileWriter
  vtkPCDWriter)

vtk_module_add_module(LidarView::IOGeneral
  CLASSES ${classes})
