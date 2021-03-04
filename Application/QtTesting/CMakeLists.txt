message(DEBUG "Add Qt test:")
include(LidarGUITesting)
#-----------------------------------------------------------------------------
# Fill this lists with tests
set(tests_with_baselines
  TestQtOrthogonalView
  TestQtNoRPMIfNoData
  TestQtOpenVLP16Dual
  TestQtOpenHDL64Dual
  TestQtShowRPMHDL64
  #TestQtOpenHDL64Livestream #TODO but hard
  )

set(tests_without_baselines
  #TestQtRulerActionDisabledOnNonOrthogonalView #Wip not meaningful, Qtoolbutton name of measureAction is random
  )
    
#-----------------------------------------------------------------------------
# Get Installation bin directory
get_filename_component(install_location "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
if(APPLE)
  set(INSTALL_LOCAL_DIR "${install_location}/bin/LidarView.app/Contents/MacOS")
else()
  set(INSTALL_LOCAL_DIR "${install_location}/bin")
endif()
#-----------------------------------------------------------------------------
# Create lists of tests
set(all_tests ${tests_without_baselines} ${tests_with_baselines} )

#[[
#-----------------------------------------------------------------------------
# Configure tests
foreach(test_name ${all_tests})
  configure_file(
    "${CMAKE_SOURCE_DIR}/Application/QtTesting/Scripts/${test_name}.xml"
    "${CMAKE_BINARY_DIR}/Application/QtTesting/Scripts/${test_name}.xml"
    )
endforeach()
]]

#-----------------------------------------------------------------------------
# Disabled QtTesting on OSX   because of complicated boost-RPATH-patch issue, This should be fixed in PV59.
# Disabled QtTesting on WIN32 because of egl-context related issue, This should be fixed in PV59
if(UNIX AND NOT APPLE)
  # Add Tests
  add_GUI_tests_without_baselines(${INSTALL_LOCAL_DIR}/LidarView "${tests_without_baselines}" "${CMAKE_SOURCE_DIR}/Plugins/VelodynePlugin/Plugin/VelodyneModule")
  add_GUI_tests_with_baselines   (${INSTALL_LOCAL_DIR}/LidarView "${tests_with_baselines}"    "${CMAKE_SOURCE_DIR}/Plugins/VelodynePlugin/Plugin/VelodyneModule")
endif()
