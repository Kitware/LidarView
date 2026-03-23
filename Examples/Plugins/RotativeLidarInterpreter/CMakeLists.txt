set(calib_files
  CalibrationExample.csv
)

paraview_add_plugin(RotativeLidarInterpreter
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "2.0"
  MODULES
    RotativeLidarInterpreter::RotativeLidarPacketInterpreter
  MODULE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/RotativeLidarPacketInterpreter/vtk.module"
  SERVER_MANAGER_XML
    "${CMAKE_CURRENT_SOURCE_DIR}/RotativeLidarExampleProxies.xml"
)

# Copy the calibration to the build folder
foreach(file ${calib_files})
  configure_file("CalibrationFiles/${file}" "${PROJECT_BINARY_DIR}/share/${file}" COPYONLY)
endforeach()

# Install step
foreach(file ${calib_files})
  install(
    FILES "${PROJECT_BINARY_DIR}/share/${file}"
    DESTINATION "share"
  )
endforeach()
