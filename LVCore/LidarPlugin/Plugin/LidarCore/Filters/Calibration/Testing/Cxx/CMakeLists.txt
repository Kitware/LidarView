vtk_add_test_cxx(lvFiltersCalibrationCxxTests tests
  NO_VALID NO_OUTPUT
  TestCameraCalibration.cxx
  TestCarGeometricCalibration-LaDoua.cxx
  TestCarGeometricCalibration-MM.cxx
  TestGeometricCalibration-LaDoua.cxx
  TestGeometricCalibration-MM.cxx
#   TestScaleCalibration-MM.cxx
  )

vtk_test_cxx_executable(lvFiltersCalibrationCxxTests tests)
