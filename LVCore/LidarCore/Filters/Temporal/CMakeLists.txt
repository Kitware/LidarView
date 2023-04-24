set(classes
  vtkAggregatePointsFromTrajectoryOffline
  vtkAggregatePointsFromTrajectoryOnline
  vtkMLSPosesSmoothing
  vtkMotionDetector
  vtkSphericalMap
  vtkTemporalTransformsApplier
  vtkTrailingFrame)

vtk_module_add_module(LidarView::FiltersTemporal
  CLASSES ${classes})
