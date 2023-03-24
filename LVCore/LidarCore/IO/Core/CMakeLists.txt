set(sources
  CategoriesConfig.cxx
  # FramesSeriesUtils.cxx
  )

set(headers
  CategoriesConfig.h
  FileSystemUtils.h
  # FramesSeriesUtils.h
  )

set(classes
  vtkBoundingBoxReader
  vtkTemporalTransformsReader
  vtkTemporalTransformsWriter)

vtk_module_add_module(LidarView::IOCore
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})
