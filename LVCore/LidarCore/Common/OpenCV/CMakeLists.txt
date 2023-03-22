set(classes
  vtkOpenCVConversions)

vtk_module_add_module(LidarView::CommonOpenCV
  FORCE_STATIC
  CLASSES ${classes})
