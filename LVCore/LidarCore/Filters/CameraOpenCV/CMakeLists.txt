set(classes
  vtkOpenCVImageMapper)

vtk_module_add_module(LidarView::FiltersCameraOpenCV
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})
