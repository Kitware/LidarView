vtk_module_add_module(LidarView::UtilitiesPCAPTools
  HEADER_ONLY)

vtk_module_add_executable(PacketFileSender
  PacketFileSender.cxx)
target_link_libraries(PacketFileSender
  PRIVATE
    LidarView::UtilitiesPCAPTools)

# Would need some refactoring
# vtk_module_add_executable(PCAPSeparator
#   PCAPSeparator.cxx)
# target_link_libraries(PCAPSeparator
#   PRIVATE
#     LidarView::UtilitiesPCAPTools)

# Would need some refactoring - uses vtkVelodynePacketInterpreter
# vtk_module_add_executable(PCAPTester
#     PCAPTester.cxx)
#   target_link_libraries(PCAPTester
#     PRIVATE
#       LidarView::UtilitiesPCAPTools)
