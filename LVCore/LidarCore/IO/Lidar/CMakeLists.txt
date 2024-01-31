set(classes
  vtkLidarPacketInterpreter
  vtkLidarReader
  vtkLidarStream
  vtkLidarPoseReader
  vtkLidarPoseStream
  vtkPosePacketInterpreter
)

vtk_module_add_module(LidarView::IOLidar
  CLASSES ${classes})
