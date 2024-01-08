set(classes
  vtkLidarPacketInterpreter
  vtkLidarReader
  vtkLidarStream
  vtkLidarPosePacketInterpreter
  vtkLidarPoseReader
  vtkLidarPoseStream
)

vtk_module_add_module(LidarView::IOLidar
  CLASSES ${classes})
