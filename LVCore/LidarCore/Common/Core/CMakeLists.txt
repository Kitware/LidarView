set(sources
  CameraProjection.cxx
  LVTime.cxx
  vtkTimeCalibration.cxx # Could not be Python wrapped
  )

set(headers
  # BoundingBox.h
  CameraProjection.h
  eigenFFTCorrelation.h
  interpolator1D.h
  LVTime.h
  statistics.h)

set(classes
  vtkConversions
  vtkCustomTransformInterpolator
  vtkEigenTools
  vtkHelper
  vtkMergePointsToPolyDataHelper
  vtkPipelineTools
  vtkPointsPCA
  vtkTemporalTransforms
  vtkVoxelGridProcessor)

set(template_classes
  vtkLiveSourceAlgorithm)

# set(templates
#   BoundingBox)

vtk_module_add_module(LidarView::CommonCore
  SOURCES ${sources}
  CLASSES ${classes}
  # TEMPLATES ${templates}
  HEADERS ${headers}
  TEMPLATE_CLASSES ${template_classes})
