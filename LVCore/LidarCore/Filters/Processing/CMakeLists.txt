set(classes
  vtkPlaneProjector
  vtkRansacPlaneModel
  vtkComputeVolume)

vtk_module_add_module(LidarView::FiltersProcessing
  CLASSES ${classes})
