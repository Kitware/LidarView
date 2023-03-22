set(classes
  vtkPlaneProjector
  vtkRansacPlaneModel)

vtk_module_add_module(LidarView::FiltersProcessing
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/PlaneProjector.xml
    Resources/RansacPlaneModel.xml)
