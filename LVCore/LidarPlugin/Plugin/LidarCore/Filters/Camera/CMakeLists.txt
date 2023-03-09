set(classes
  vtkCameraMapper
  vtkCameraProjector)

vtk_module_add_module(LidarView::FiltersCamera
  FORCE_STATIC
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/CameraMapper.xml
    Resources/CameraProjector.xml)
