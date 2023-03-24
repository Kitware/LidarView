set(sources
  CameraCalibration.cxx)

set(headers
  CameraCalibration.h)

set(classes
  vtkCalibrationFromPoses
  vtkCarGeometricCalibration
  vtkTemporalTransformsRemapper)

vtk_module_add_module(LidarView::FiltersCalibration
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})
