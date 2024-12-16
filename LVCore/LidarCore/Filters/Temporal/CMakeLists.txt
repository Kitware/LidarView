set(classes
  vtkAggregatePointsFromTrajectoryOffline
  vtkAggregatePointsFromTrajectoryOnline
  vtkClusteringAndTracking
  vtkMLSPosesSmoothing
  vtkMotionDetector
  vtkTemporalTransformsApplier
  vtkTrailingFrame)

vtk_module_add_module(LidarView::FiltersTemporal
  CLASSES ${classes})
