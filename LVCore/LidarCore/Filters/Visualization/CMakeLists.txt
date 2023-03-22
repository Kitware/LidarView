set(classes
  vtkBirdEyeViewSnap
  vtkLidarRawSignalImage
  vtkPointCloudLinearProjector)

vtk_module_add_module(LidarView::FiltersVisualization
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/BirdEyeViewSnap.xml
    Resources/LidarRawSignalImage.xml
    Resources/PointCloudLinearProjector.xml)
