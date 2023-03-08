set(classes
  vtkBirdEyeViewSnap)

vtk_module_add_module(LidarView::FiltersVisualization
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/BirdEyeViewSnap.xml)
