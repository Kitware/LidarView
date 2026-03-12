set(classes
  vtkTemporalTransformsReader
  vtkTemporalTransformsWriter)

vtk_module_add_module(LidarView::IOCore
  CLASSES ${classes})
