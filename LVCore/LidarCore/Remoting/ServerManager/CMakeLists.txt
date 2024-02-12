set(classes
  vtkSMInterpretersManagerProxy
  vtkSMLidarReaderProxy
)

vtk_module_add_module(LidarView::RemotingServerManager
  CLASSES ${classes})
