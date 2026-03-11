set(classes
  vtkCameraMapper
  vtkLaplacianInfilling)

vtk_module_add_module(LidarView::FiltersCamera
  CLASSES ${classes})
