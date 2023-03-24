set(classes
  vtkPlaneProjector
  vtkRansacPlaneModel)

vtk_module_add_module(LidarView::FiltersProcessing
  CLASSES ${classes})
