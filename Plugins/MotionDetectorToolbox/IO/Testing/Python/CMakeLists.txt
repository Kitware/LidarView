set(_vtk_testing_python_exe "$<TARGET_FILE:LidarView::lvpython>")

vtk_add_test_python(
  TestDetectedClusterUDPSender.py,NO_OUTPUT,NO_DATA,NO_VALID
)
