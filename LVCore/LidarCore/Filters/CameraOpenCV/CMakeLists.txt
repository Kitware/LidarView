set(classes
  vtkOpenCVImageMapper
  vtkCameraProjector)

vtk_module_add_module(LidarView::FiltersCameraOpenCV
  CLASSES ${classes})
