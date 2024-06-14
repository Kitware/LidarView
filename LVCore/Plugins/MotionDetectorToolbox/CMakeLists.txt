paraview_add_plugin(MotionDetectorToolbox
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES
    MotionDetectorToolbox::IO
  MODULE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/IO/vtk.module"
)

vtk_module_wrap_python(
  MODULES             MotionDetectorToolbox::IO
  INSTALL_EXPORT      LidarViewPython
  INSTALL_HEADERS     "${LIDARVIEW_INSTALL_DEVELOPMENT_FILES}"
  CMAKE_DESTINATION   "${lidarview_cmake_destination}"
  LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lidarview/lvpythonmodules"
  PYTHON_PACKAGE      "lidarview.modules"
  MODULE_DESTINATION  "${LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX}"
  TARGET              MotionDetectorToolbox::pythonmodules
  DEPENDS             VTK::vtkpythonmodules
)
