set(headers
  vtkLidarViewDeprecation.h
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkLVVersion.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkLVVersion.h"
  @ONLY)
list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkLVVersion.h")

vtk_module_add_module(LidarView::Versioning
  HEADERS ${headers}
  HEADER_ONLY)
