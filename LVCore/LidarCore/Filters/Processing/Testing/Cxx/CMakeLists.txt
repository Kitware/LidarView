vtk_add_test_cxx(lvFiltersProcessingCxxTests tests
  NO_VALID NO_OUTPUT
  TestRansacPlaneModel.cxx
  TestComputeVolume.cxx)

vtk_test_cxx_executable(lvFiltersProcessingCxxTests tests)
