set(classes
  vtkPCDWriter
  vtkExternalSensorReader)

vtk_module_add_module(LidarView::IOGeneral
  CLASSES ${classes})
