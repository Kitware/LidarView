set(classes
  vtkBirdEyeViewSnap
  vtkLidarRawSignalImage
  vtkPointCloudLinearProjector)

vtk_module_add_module(LidarView::FiltersVisualization
  CLASSES ${classes})
