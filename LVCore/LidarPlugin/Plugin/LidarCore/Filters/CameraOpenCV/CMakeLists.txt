set(sources
  MIDHOG.cxx)

set(headers
  MIDHOG.h)

set(classes
  vtkOpenCVImageMapper)

vtk_module_add_module(LidarView::FiltersCameraOpenCV
  FORCE_STATIC
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})

paraview_add_server_manager_xmls(
  XMLS
    Resources/OpenCVImageMapper.xml)
