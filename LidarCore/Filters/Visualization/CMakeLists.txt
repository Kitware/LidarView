set(classes
  vtkBirdEyeViewSnap
  vtkColorPointNeighborImage
  vtkImageColorToPointCloud
  vtkLidarRawSignalImage
  vtkPointCloudLinearProjector
  vtkPointCloudPinHoleProjector
  vtkVoxelGridFilter)

vtk_module_add_module(LidarView::FiltersVisualization
  CLASSES ${classes})
