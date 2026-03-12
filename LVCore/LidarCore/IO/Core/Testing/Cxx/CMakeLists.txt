vtk_add_test_cxx(lvIOCoreCxxTests tests
  NO_VALID
  TestTemporalTransformsReaderWriter.cxx
  # TestTimeCalibrationLaDoua.cxx
  TestTimeCalibrationMM.cxx
)

vtk_test_cxx_executable(lvIOCoreCxxTests tests)
