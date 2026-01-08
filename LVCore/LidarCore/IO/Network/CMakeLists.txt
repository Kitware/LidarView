set(sources
  InterpreterHelper.cxx
  UDPPacketSender.cxx
)

set(headers
  InterpreterHelper.h
  UDPPacketSender.h
  vtkStreamPacketHandler.h
)

set(templates
  vtkStreamPacketHandler.txx
)

set(classes
  vtkInterpreter
  vtkPacketFileHandler
  vtkPacketRecorder
  vtkPCAPVersion
  vtkStream
  vtkStreamPacketSniffer
  vtkTCPPacketReceiver
  vtkTCPPacketSender
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
  TEMPLATES ${templates}
  PRIVATE_HEADERS ${private_headers}
  PRIVATE_TEMPLATES ${private_template}
  PRIVATE_CLASSES ${private_classes}
)
