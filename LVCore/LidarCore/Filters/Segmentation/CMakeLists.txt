set(sources)
set(headers)
set(classes
  vtkCurbDetectorOffline
)

if (LIDARVIEW_USE_YAMLCPP AND LIDARVIEW_USE_NANOFLANN)
  list(APPEND headers
    CategoriesConfig.h
    SegmentedCloudTransformations.h
    KDTreeVectorOfVectorsAdaptor.h
  )
  list(APPEND sources
    CategoriesConfig.cxx
    SegmentedCloudTransformations.cxx
  )
  list(APPEND classes
    vtkDBSCANClustering
    vtkSeparateCloudKnn
  )
endif ()

# set(templates
#   DBSCAN)

vtk_module_add_module(LidarView::FiltersSegmentation
  SOURCES ${sources}
  CLASSES ${classes}
  # TEMPLATES ${templates}
  HEADERS ${headers})

if (LIDARVIEW_USE_YAMLCPP AND LIDARVIEW_USE_NANOFLANN)
  vtk_module_link(LidarView::FiltersSegmentation
    PUBLIC
      nanoflann::nanoflann
      YAML::yamlcpp
  )
endif ()
