function (lidarview_add_test_python)
  # LidarView::lvpython is common for all LidarView based app
  set(_vtk_testing_python_exe "$<TARGET_FILE:LidarView::lvpython>")
  vtk_add_test_python(${ARGN})
endfunction ()

lidarview_add_test_python(
    NO_RT
    TestResetCamera.py
)
