set(classes
  vtkExtractPointSelection)

vtk_module_add_module(LidarView::FiltersSelection
  FORCE_STATIC
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/ExtractPointSelection.xml)
