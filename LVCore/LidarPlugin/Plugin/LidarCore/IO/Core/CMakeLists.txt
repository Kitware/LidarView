set(sources
  CategoriesConfig.cxx
  # FramesSeriesUtils.cxx
  )

set(headers
  CategoriesConfig.h
  FileSystemUtils.h
  FrameInformation.h
  # FramesSeriesUtils.h
  InterpreterHelper.h)

set(classes
  vtkBoundingBoxReader
  vtkTemporalTransformsReader
  vtkTemporalTransformsWriter)

vtk_module_add_module(LidarView::IOCore
  SOURCES ${sources}
  CLASSES ${classes}
  HEADERS ${headers})

paraview_add_server_manager_xmls(
  XMLS
    Resources/BoundingBoxReader.xml
    Resources/TemporalTransformsReader.xml
    Resources/TemporalTransformsWriter.xml)
