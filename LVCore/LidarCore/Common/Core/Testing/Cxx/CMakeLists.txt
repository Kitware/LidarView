vtk_add_test_cxx(lvCommonCoreCxxTests tests
  NO_VALID NO_OUTPUT
  TestBoundingBox.cxx
  TestVtkCustomTransformInterpolator.cxx
  # TestVtkEigenTools.cxx
  TestVtkMergePointsToPolyDataHelper.cxx
  TestVtkVoxelGridProcessor.cxx
  )

vtk_test_cxx_executable(lvCommonCoreCxxTests tests)
