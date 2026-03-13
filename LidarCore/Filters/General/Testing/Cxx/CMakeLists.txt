vtk_add_test_cxx(lvFiltersGeneralCxxTests tests
  NO_VALID NO_OUTPUT
  TestSelectPointsByScalars.cxx
)

vtk_test_cxx_executable(lvFiltersGeneralCxxTests tests)
