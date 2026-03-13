set(classes
  vtkAggregatePointsFromTrajectoryOffline
  vtkAggregatePointsFromTrajectoryOnline
  vtkAssociateTableToTemporalDataset
  vtkClusteringAndTracking
  vtkMLSPosesSmoothing
  vtkMotionDetector
  vtkTemporalTransformsApplier
  vtkTrailingFrame)

vtk_module_add_module(LidarView::FiltersTemporal
  CLASSES ${classes})

if (LIDARVIEW_USE_NANOFLANN)
  vtk_module_link(LidarView::FiltersTemporal
    PRIVATE
      nanoflann::nanoflann
  )
endif()
