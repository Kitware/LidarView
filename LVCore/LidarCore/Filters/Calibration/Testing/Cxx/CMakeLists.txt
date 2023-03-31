vtk_add_test_cxx(lvFiltersCalibrationCxxTests tests
  NO_VALID NO_OUTPUT
  TestCameraCalibration.cxx
  TestCarGeometricCalibrationLaDoua.cxx
  TestCarGeometricCalibrationMM.cxx
  TestGeometricCalibrationLaDoua.cxx
  TestGeometricCalibrationMM.cxx
  TestScaleCalibrationMM.cxx
)

vtk_test_cxx_executable(lvFiltersCalibrationCxxTests tests)
