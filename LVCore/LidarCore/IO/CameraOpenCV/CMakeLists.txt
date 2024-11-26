set(headers
  FrameInformation.h
)

set(classes
  vtkOpenCVVideoReader
  vtkPCAPImageReader)

vtk_module_add_module(LidarView::IOCameraOpenCV
  HEADERS ${headers}
  CLASSES ${classes})
