vtk_add_test_cxx(lvIOGeolocationCxxTests tests
  NO_VALID NO_OUTPUT
  TestNMEAParser.cxx)

vtk_test_cxx_executable(lvIOGeolocationCxxTests tests)
