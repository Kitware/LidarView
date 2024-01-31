set(sources
  CameraModel.cxx
)

set(headers
  CameraModel.h
)

vtk_module_add_module(LidarView::CommonYaml
  SOURCES ${sources}
  HEADERS ${headers}
)
