set(classes
  vtkBirdEyeViewSnap)

vtk_module_add_module(LidarView::FiltersVisualization
  FORCE_STATIC
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/BirdEyeViewSnap.xml)
