set(sources
  PacketSender.cxx
)

set(headers
  InterpreterHelper.h
  PacketSender.h
  vtkStreamPacketHandler.h
)

set(templates
  vtkStreamPacketHandler.txx
)

set(classes
  vtkInterpreter
  vtkPacketFileHandler
  vtkPacketRecorder
  vtkStream
  vtkStreamPacketSniffer
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
  vtkPacketFilePositionHandler
  vtkUDPReceiverSocketImpl
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPacketFilePositionType.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPacketFilePositionType.h"
  @ONLY)
list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPacketFilePositionType.h")

vtk_module_add_module(LidarView::IONetwork
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers}
  TEMPLATES ${templates}
  PRIVATE_HEADERS ${private_headers}
  PRIVATE_TEMPLATES ${private_template}
  PRIVATE_CLASSES ${private_classes}
)
