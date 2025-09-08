set(classes
  vtkRPMText
  vtkSelectPointsByScalars)

vtk_module_add_module(LidarView::FiltersGeneral
  CLASSES ${classes})
