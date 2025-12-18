set(classes
  vtkClusterText
  vtkRPMText
  vtkSelectPointsByScalars
  vtkNormalizeExternalSensorData)

vtk_module_add_module(LidarView::FiltersGeneral
  CLASSES ${classes})
