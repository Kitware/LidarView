vtk_add_test_cxx(lvFiltersGeneralCxxTests tests
  NO_VALID NO_OUTPUT
  TestINSTableToPoses.cxx
  TestSelectPointsByScalars.cxx
)

vtk_test_cxx_executable(lvFiltersGeneralCxxTests tests)
