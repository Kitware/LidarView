set(headers
  FrameInformation.h
)

set(classes
  vtkOpenCVVideoReader)

vtk_module_add_module(LidarView::IOCameraOpenCV
  HEADERS ${headers}
  CLASSES ${classes})
