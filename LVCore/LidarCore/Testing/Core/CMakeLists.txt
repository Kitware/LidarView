set(classes
  TestHelpers)

vtk_module_add_module(LidarView::TestingCore
  FORCE_STATIC
  CLASSES ${classes})
