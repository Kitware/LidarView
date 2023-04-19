set(classes
  vtkBirdEyeViewSnap
  vtkLidarRawSignalImage
  vtkPointCloudLinearProjector
  vtkVoxelGridFilter)

vtk_module_add_module(LidarView::FiltersVisualization
  CLASSES ${classes})
