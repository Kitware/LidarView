set(sources
  KalmanTracker.cxx)

set(private_headers
  KalmanTracker.h)

set(classes
  vtkAggregatePointsFromTrajectoryOffline
  vtkAggregatePointsFromTrajectoryOnline
  vtkAssociateTableToTemporalDataset
  vtkClusteringAndTracking
  vtkKalmanTracker
  vtkMLSPosesSmoothing
  vtkMotionDetector
  vtkTemporalTransformsApplier
  vtkTrailingFrame)

vtk_module_add_module(LidarView::FiltersTemporal
  SOURCES ${sources}
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})

if (LIDARVIEW_USE_NANOFLANN)
  vtk_module_link(LidarView::FiltersTemporal
    PRIVATE
      nanoflann::nanoflann
  )
endif()
