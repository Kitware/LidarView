set(classes
  vtkBirdEyeViewSnap
  vtkLidarRawSignalImage
  vtkPointCloudLinearProjector
  vtkVoxelGridFilter
  vtkPointCloudPinHoleProjector
  vtkColorPointNeighborImage)

vtk_module_add_module(LidarView::FiltersVisualization
  CLASSES ${classes})
