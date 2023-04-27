set(calib_files
  CalibrationExample.csv
)

paraview_add_plugin(InterpreterExamplePlugin
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "2.0"
  MODULES
    InterpreterExamplePlugin::ExamplePacketInterpreters
  MODULE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ExamplePacketInterpreters/vtk.module"
)

# Copy the calibration to the build folder
foreach(file ${calib_files})
  configure_file("CalibrationFiles/${file}" "${PROJECT_BINARY_DIR}/share/${file}" COPYONLY)
endforeach()

# Install step
set(install_calib_files_dest share)
if(APPLE)
  set(install_calib_files_dest "${LV_INSTALL_LIBRARY_DIR}/../Resources")
endif()
foreach(file ${calib_files})
  install(FILES "${PROJECT_BINARY_DIR}/share/${file}" DESTINATION ${install_calib_files_dest})
endforeach()
