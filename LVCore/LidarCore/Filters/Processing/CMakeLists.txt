set(classes
  vtkPlaneProjector
  vtkRansacPlaneModel
  vtkComputeVolume)

set(private_headers)
set(sources)

if (LIDARVIEW_USE_NANOFLANN)
  list(APPEND classes vtkComputeNormalAdaptive)

  list(APPEND private_headers
      KDTreeVTKAdaptor.h
      NeighborsSearch.h
      NormalsEstimator.h)

  list(APPEND sources
      NormalsEstimator.cxx
      NeighborsSearch.cxx)
endif()

vtk_module_add_module(LidarView::FiltersProcessing
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers}
  SOURCES ${sources})
