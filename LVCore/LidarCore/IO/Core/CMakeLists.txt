set(sources
  CategoriesConfig.cxx
  )

set(headers
  CategoriesConfig.h
  )

set(classes
  vtkBoundingBoxReader
  vtkTemporalTransformsReader
  vtkTemporalTransformsWriter)

vtk_module_add_module(LidarView::IOCore
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})
