set(classes
  vtkThresholdFromFile)

vtk_module_add_module(ThresholdFromFile
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  ThresholdFromFile.xml)
