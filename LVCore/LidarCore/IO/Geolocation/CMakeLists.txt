set(sources
  GPSProjectionUtils.cxx
  NMEAParser.cxx
  )

set(headers
  GPSProjectionUtils.h
  NMEAParser.h)

set(classes
  vtkApplanixPositionReader
  vtkArduPilotDataFlashLogReader
  vtkPositionOrientationPacketInterpreter
  vtkPositionOrientationPacketReader
  vtkPositionOrientationStream)

vtk_module_add_module(LidarView::IOGeolocation
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})

paraview_add_server_manager_xmls(
  XMLS
    Resources/ApplanixPositionReader.xml
    Resources/ArduPilotDataFlashLogReader.xml
    Resources/PositionOrientationPacketInterpreter.xml
    Resources/PositionOrientationPacketReader.xml
    Resources/PositionOrientationStream.xml)
