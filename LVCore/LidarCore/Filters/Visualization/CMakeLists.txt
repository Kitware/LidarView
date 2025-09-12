set(classes
  vtkBirdEyeViewSnap
  vtkLidarRawSignalImage
  vtkPointCloudLinearProjector
  vtkVoxelGridFilter
  vtkPointCloudPinHoleProjector)

vtk_module_add_module(LidarView::FiltersVisualization
  CLASSES ${classes})
