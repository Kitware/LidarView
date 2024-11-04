set(sources
  CrashAnalysing.cxx
  NetworkPacket.cxx
  PacketSender.cxx
)

set(headers
  CrashAnalysing.h
  FrameInformation.h
  InterpreterHelper.h
  NetworkPacket.h
  PacketSender.h
  vtkStreamPacketHandler.h
)

set(classes
  vtkInterpreter
  vtkPacketFileReader
  vtkPacketFileWriter
  vtkPacketRecorder
  vtkStream
  vtkUDPPacketReceiver
  vtkUDPPing
  vtkUDPPointSender
  vtkUDPSenderAlgorithm
)

set(private_headers
  vtkSynchronizedQueue.h
)

set(private_template
  vtkSynchronizedQueue.txx
)

set(private_classes
  vtkUDPReceiverSocketImpl
)

vtk_module_add_module(LidarView::IONetwork
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers}
  PRIVATE_TEMPLATES ${private_template}
  PRIVATE_CLASSES ${private_classes}
)
