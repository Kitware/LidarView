set(sources
  PacketSender.cxx
)

set(headers
  InterpreterHelper.h
  PacketSender.h
  vtkStreamPacketHandler.h
)

set(classes
  vtkInterpreter
  vtkPacketFileHandler
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
