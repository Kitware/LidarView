set(example_classes
  vtkRotativeLidarPacketInterpreter
)

set(example_headers
  RotativeLidarFormat.h
)

vtk_module_add_module(RotativeLidarInterpreter::RotativeLidarPacketInterpreter
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading
  CLASSES ${example_classes}
  HEADERS ${example_headers}
)

paraview_add_server_manager_xmls(
  MODULE RotativeLidarInterpreter::RotativeLidarPacketInterpreter
  XMLS
    RotativeLidarPacketInterpreter.xml
    RotativeLidarReader.xml
    RotativeLidarStream.xml
)
