set(headers
  CeresCameraCalibrationCostFunctions.h
  CeresCostFunctions.h
  CeresTools.h)

set(classes
  vtkGeometricCalibration)

vtk_module_add_module(LidarView::CommonCeres
  CLASSES ${classes}
  HEADERS ${headers})
