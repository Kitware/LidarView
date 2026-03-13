set(headers)
set(classes
  vtkDummyExternalAPI
)

find_package(ZeroMQ)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/zmqInfo.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/zmqInfo.h"
  @ONLY)
list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/zmqInfo.h")

vtk_module_add_module(CustomDockWidget::DummyExternalAPI
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading
  CLASSES ${classes}
  PRIVATE_HEADERS ${headers}
)

if (ZeroMQ_FOUND)
  vtk_module_link(CustomDockWidget::DummyExternalAPI PRIVATE libzmq)
endif ()
