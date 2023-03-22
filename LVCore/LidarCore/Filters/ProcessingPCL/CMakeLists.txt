set(classes
  vtkPCLRansacModel)

vtk_module_add_module(LidarView::FiltersProcessingPCL
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/PCLRansacModel.xml)
