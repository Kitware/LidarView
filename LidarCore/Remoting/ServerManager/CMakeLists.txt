set(classes
  vtkSMInterpretersManagerProxy
  vtkSMLidarProxy
  vtkSMLidarReaderProxy
  vtkSMLidarStreamProxy
)

vtk_module_add_module(LidarView::RemotingServerManager
  CLASSES ${classes})
