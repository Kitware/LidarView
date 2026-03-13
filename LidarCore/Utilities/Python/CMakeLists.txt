configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkLVPythonConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkLVPythonConfigure.h")

set(headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkLVPythonConfigure.h")

set(classes
  vtkLVPython)

vtk_module_add_module(LidarView::PythonInitializer
  HEADERS ${headers}
  CLASSES ${classes})
