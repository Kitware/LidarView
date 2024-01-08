set(sources
  GPSProjectionUtils.cxx
  NMEAParser.cxx
  )

set(headers
  GPSProjectionUtils.h
  NMEAParser.h)

set(classes
  vtkApplanixPositionReader
  vtkArduPilotDataFlashLogReader)

vtk_module_add_module(LidarView::IOGeolocation
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})
