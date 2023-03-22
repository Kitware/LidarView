set(classes
  vtkMLSPosesSmoothing
  vtkMotionDetector
  vtkSphericalMap
  vtkTemporalTransformsApplier
  vtkTrailingFrame)

vtk_module_add_module(LidarView::FiltersTemporal
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/MLSPosesSmoothing.xml
    Resources/MotionDetector.xml
    Resources/TemporalTransformsApplier.xml
    Resources/TrailingFrame.xml)
