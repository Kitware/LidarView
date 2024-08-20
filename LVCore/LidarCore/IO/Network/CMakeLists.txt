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
  vtkStream
  vtkUDPPing
  vtkUDPPointSender
  vtkUDPSenderAlgorithm
)

vtk_module_add_module(LidarView::IONetwork
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})
