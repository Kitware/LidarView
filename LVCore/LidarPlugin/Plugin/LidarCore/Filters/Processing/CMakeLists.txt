set(classes
  vtkLaplacianInfilling
  vtkLidarRawSignalImage
  vtkPlaneProjector
  vtkPointCloudLinearProjector
  vtkRansacPlaneModel)

vtk_module_add_module(LidarView::FiltersProcessing
  FORCE_STATIC
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS
    Resources/LaplacianInfilling.xml
    Resources/LidarRawSignalImage.xml
    Resources/PlaneProjector.xml
    Resources/PointCloudLinearProjector.xml
    Resources/RansacPlaneModel.xml)
