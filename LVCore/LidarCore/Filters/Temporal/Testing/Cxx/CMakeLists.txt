vtk_add_test_cxx(lvFiltersTemporalCxxTests tests
  NO_VALID NO_OUTPUT
  TestTrailingFrame.cxx
  TestAssociatedTableToTemporalDataset.cxx)

vtk_test_cxx_executable(lvFiltersTemporalCxxTests tests)
