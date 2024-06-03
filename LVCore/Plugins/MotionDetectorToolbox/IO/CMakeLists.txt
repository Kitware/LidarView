set(classes
  vtkDetectedClusterUDPSender
)

vtk_module_add_module(MotionDetectorToolbox::IO
  CLASSES ${classes}
)

paraview_add_server_manager_xmls(
  MODULE MotionDetectorToolbox::IO
  XMLS
    DetectedClusterUDPSender.xml
)
