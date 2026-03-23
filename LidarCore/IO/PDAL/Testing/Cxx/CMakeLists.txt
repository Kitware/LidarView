vtk_add_test_cxx(lvLASWriterCxxTests tests
  NO_VALID NO_OUTPUT
  TestLASWriter.cxx
)

vtk_test_cxx_executable(lvLASWriterCxxTests tests)