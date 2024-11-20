set(example_classes
  vtkRotativeLidarPacketInterpreter
)

set(example_headers
  RotativeLidarFormat.h
)

# Build the vtk module library with interpreter class
vtk_module_add_module(RotativeLidarInterpreter::RotativeLidarPacketInterpreter
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading
  CLASSES ${example_classes}
  HEADERS ${example_headers}
)

# Add xml proxies file to paraview server manager
paraview_add_server_manager_xmls(
  MODULE RotativeLidarInterpreter::RotativeLidarPacketInterpreter
  XMLS
    RotativeLidarPacketInterpreter.xml
    RotativeLidarReader.xml
    RotativeLidarStream.xml
)
