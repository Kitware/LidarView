set(classes
  vtkCameraMapper
  vtkCameraProjector
  vtkLaplacianInfilling)

vtk_module_add_module(LidarView::FiltersCamera
  FORCE_STATIC
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/CameraMapper.xml
    Resources/CameraProjector.xml
    Resources/LaplacianInfilling.xml)
