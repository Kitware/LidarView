set(classes
  TestHelpers
  vtkLidarTestTools
)

vtk_module_add_module(LidarView::TestingCore
  FORCE_STATIC
  CLASSES ${classes})
