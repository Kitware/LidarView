set(classes
  vtkOpenCVVideoReader
  vtkPCAPImageReader)

vtk_module_add_module(LidarView::IOCameraOpenCV
  FORCE_STATIC
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/OpenCVVideoReader.xml
    Resources/PCAPImageReader.xml)
