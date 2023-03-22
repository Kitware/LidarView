set(sources
  SegmentedCloudTransformations.cxx)

set(headers
  # DBSCAN.h
  KDTreeVectorOfVectorsAdaptor.h
  SegmentedCloudTransformations.h)

set(classes
  vtkDBSCANClustering
  vtkSeparateCloudKnn)

# set(templates
#   DBSCAN)

vtk_module_add_module(LidarView::FiltersSegmentation
  SOURCES ${sources}
  CLASSES ${classes}
  # TEMPLATES ${templates}
  HEADERS ${headers})

paraview_add_server_manager_xmls(
  XMLS
    Resources/DBSCANClustering.xml
    Resources/SeparateCloudKnn.xml)
