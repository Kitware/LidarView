set(example_classes
  vtkTimeBasedLidarPacketInterpreter
)

set(example_headers
  TimeBasedLidarFormat.h
)

# Build the vtk module library with interpreter class
vtk_module_add_module(TimeBasedLidarInterpreter::TimeBasedLidarPacketInterpreter
  CLASSES ${example_classes}
  HEADERS ${example_headers}
)

# Add xml proxies file to paraview server manager
paraview_add_server_manager_xmls(
  MODULE TimeBasedLidarInterpreter::TimeBasedLidarPacketInterpreter
  XMLS
    TimeBasedLidarPacketInterpreter.xml
    TimeBasedLidarReader.xml
    TimeBasedLidarStream.xml
)
