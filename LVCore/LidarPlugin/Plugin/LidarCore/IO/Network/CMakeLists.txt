set(sources
  CrashAnalysing.cxx
  NetworkPacket.cxx
  PacketConsumer.cxx
  PacketFileWriter.cxx
  PacketReceiver.cxx
  PacketSender.cxx)

set(headers
  CrashAnalysing.h
  FrameInformation.h
  InterpreterHelper.h
  NetworkPacket.h
  PacketConsumer.h
  PacketFileWriter.h
  PacketReceiver.h
  PacketSender.h
  SynchronizedQueue.h)

set(classes
  vtkInterpreter
  vtkPacketFileReader
  vtkPacketFileWriter
  vtkStream)

vtk_module_add_module(LidarView::IONetwork
  FORCE_STATIC
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})

paraview_add_server_manager_xmls(
  XMLS
    Resources/Interpreter.xml
    Resources/Stream.xml)
