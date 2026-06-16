set(classes
  vtkBoundingBoxReader)

vtk_module_add_module(LidarView::IOYaml
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})
