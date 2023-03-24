set(classes
  vtkOpenCVVideoReader
  vtkPCAPImageReader)

vtk_module_add_module(LidarView::IOCameraOpenCV
  CLASSES ${classes})
