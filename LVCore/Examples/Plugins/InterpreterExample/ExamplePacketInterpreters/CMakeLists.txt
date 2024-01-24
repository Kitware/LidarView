set(example_classes
  vtkExamplePacketInterpreter
)

set(example_headers
  ExampleFormat.h
)

vtk_module_add_module(InterpreterExamplePlugin::ExamplePacketInterpreters
  CLASSES ${example_classes}
  HEADERS ${example_headers}
)

paraview_add_server_manager_xmls(
  MODULE InterpreterExamplePlugin::ExamplePacketInterpreters
  XMLS
    ExamplePacketInterpreter.xml
    ExampleLidarReader.xml
    ExampleLidarStream.xml
)
