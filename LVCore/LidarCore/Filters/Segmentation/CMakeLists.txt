set(sources
  CategoriesConfig.cxx
  SegmentedCloudTransformations.cxx)

set(headers
  CategoriesConfig.h
  # DBSCAN.h
  KDTreeVectorOfVectorsAdaptor.h
  SegmentedCloudTransformations.h)

set(classes
  vtkDBSCANClustering
  vtkSeparateCloudKnn
  vtkCurbDetectorOffline)

# set(templates
#   DBSCAN)

vtk_module_add_module(LidarView::FiltersSegmentation
  SOURCES ${sources}
  CLASSES ${classes}
  # TEMPLATES ${templates}
  HEADERS ${headers})
