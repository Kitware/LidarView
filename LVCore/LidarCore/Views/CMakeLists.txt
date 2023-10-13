set(classes
  vtkLidarGridActor
  vtkLidarGridView)

vtk_module_add_module(LidarView::Views
  CLASSES ${classes})
