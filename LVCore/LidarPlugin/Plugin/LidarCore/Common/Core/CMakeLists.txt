set(sources
  CameraModel.cxx
  CameraProjection.cxx
  LVTime.cxx
  vtkTimeCalibration.cxx # Could not be Python wrapped
  )

set(headers
  # BoundingBox.h
  CameraModel.h
  CameraProjection.h
  eigenFFTCorrelation.h
  interpolator1D.h
  LVTime.h
  statistics.h)

set(classes
  vtkCarGeometricCalibration
  vtkConversions
  vtkCustomTransformInterpolator
  vtkEigenTools
  vtkHelper
  vtkPipelineTools
  vtkTemporalTransforms)

# set(templates
#   BoundingBox)

vtk_module_add_module(LidarView::CommonCore
  SOURCES ${sources}
  CLASSES ${classes}
  # TEMPLATES ${templates}
  HEADERS ${headers})
