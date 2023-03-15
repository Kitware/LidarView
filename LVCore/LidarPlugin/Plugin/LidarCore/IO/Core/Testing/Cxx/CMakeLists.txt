vtk_add_test_cxx(lvIOCoreCxxTests tests
  NO_VALID NO_OUTPUT
  TestTemporalTransformsReaderWriter.cxx
#   TestTimeCalibration-LaDoua.cxx
#   TestTimeCalibration-MM.cxx
  )

vtk_test_cxx_executable(lvIOCoreCxxTests tests)
