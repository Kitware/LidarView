set(classes
  vtkGridSource)

vtk_module_add_module(LidarView::Sources
  FORCE_STATIC
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/GridSource.xml)
