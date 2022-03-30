# Sanitize checks
if(NOT LV_INSTALL_LIBRARY_DIR )
  message(FATAL_ERROR "LV_INSTALL_LIBRARY_DIR not set")
endif()

if (WIN32)
  get_filename_component(TEST_BINARY_DIR "${CMAKE_INSTALL_PREFIX}/bin" ABSOLUTE) #wip not tested
else()
  set(TEST_BINARY_DIR "${CMAKE_BINARY_DIR}/${LV_PLUGIN_INSTALL_SUBDIRECTORY}/LidarPlugin")
endif()

set(data_dir ${CMAKE_CURRENT_SOURCE_DIR}/Data)

function(custom_add_executable)
  add_executable(${ARGV})
  if (WIN32)
    install(TARGETS ${ARGV0}
	    RUNTIME DESTINATION ${TEST_BINARY_DIR}
	    LIBRARY DESTINATION ${LV_INSTALL_LIBRARY_DIR}
	  )
  endif(WIN32)
endfunction(custom_add_executable)

# Add TestHelpers
target_sources(LidarCore PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/TestHelpers.cxx
  )
target_include_directories(LidarCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Default Tests
custom_add_executable(TestNMEAParser TestNMEAParser.cxx)
target_link_libraries(TestNMEAParser LidarCore)

custom_add_executable(TestTrailingFrame TestTrailingFrame.cxx)
target_link_libraries(TestTrailingFrame LidarCore)

custom_add_executable(TestRansacPlaneModel TestRansacPlaneModel.cxx)
target_link_libraries(TestRansacPlaneModel LidarCore)

#custom_add_executable(TestVtkEigenTools TestVtkEigenTools.cxx )
#target_link_libraries(TestVtkEigenTools LidarCore)
#add_test(TestVtkEigenTools
#  ${TEST_BINARY_DIR}/TestVtkEigenTools
#)

custom_add_executable(TestBoundingBox TestBoundingBox.cxx)
target_link_libraries(TestBoundingBox LidarCore)

custom_add_executable(TestTemporalTransformsReaderWriter TestTemporalTransformsReaderWriter.cxx )
target_link_libraries(TestTemporalTransformsReaderWriter LidarCore)

add_test(TestNMEAParser
  ${TEST_BINARY_DIR}/TestNMEAParser
)

add_test(TestTrailingFrame
  ${TEST_BINARY_DIR}/TestTrailingFrame
)

add_test(TestRansacPlaneModel
  ${TEST_BINARY_DIR}/TestRansacPlaneModel
)

add_test(TestTemporalTransformsReaderWriter
  ${TEST_BINARY_DIR}/TestTemporalTransformsReaderWriter
  ${data_dir}/trajectories/mm04/orbslam2-no-loop-closure.csv
  ${data_dir}/trajectories/mm04/orbslam2-no-loop-closure.csv.temporary
)

add_test(TestBoundingBox
  ${TEST_BINARY_DIR}/TestBoundingBox
)

#custom_add_executable(TestScaleCalibration-MM TestScaleCalibration-MM.cxx)
#target_link_libraries(TestScaleCalibration-MM LidarCore)
#add_test(TestScaleCalibration-MM
#  ${TEST_BINARY_DIR}/TestScaleCalibration-MM
#  ${data_dir}/trajectories
#)

# Comment this test are they randomly fail on osx (error and even segfault)
#custom_add_executable(TestTimeCalibration-LaDoua TestTimeCalibration-LaDoua.cxx)
#target_link_libraries(TestTimeCalibration-LaDoua LidarCore)
#add_test(TestTimeCalibration-LaDoua
#  ${TEST_BINARY_DIR}/TestTimeCalibration-LaDoua
#  ${data_dir}/trajectories/la_doua_dataset
#)

#custom_add_executable(TestTimeCalibration-MM TestTimeCalibration-MM.cxx)
#target_link_libraries(TestTimeCalibration-MM LidarCore)

#add_test(TestTimeCalibration-MM
#  ${TEST_BINARY_DIR}/TestTimeCalibration-MM
#  ${data_dir}/trajectories
#)

# Optional Tests
if(LIDARVIEW_BUILD_CALIBFROMPOSES)
  
  custom_add_executable(TestCameraCalibration TestCameraCalibration.cxx)
  target_link_libraries(TestCameraCalibration LidarCore)
  add_test(TestCameraCalibration
    ${TEST_BINARY_DIR}/TestCameraCalibration
    ${data_dir}/Camera/MatchedPoints_3D_2D
  )

  custom_add_executable(TestCarGeometricCalibration-MM TestCarGeometricCalibration-MM.cxx)
  target_link_libraries(TestCarGeometricCalibration-MM LidarCore)
  add_test(TestCarGeometricCalibration-MM
    ${TEST_BINARY_DIR}/TestCarGeometricCalibration-MM
    ${data_dir}/trajectories
  )

  custom_add_executable(TestCarGeometricCalibration-LaDoua TestCarGeometricCalibration-LaDoua.cxx)
  target_link_libraries(TestCarGeometricCalibration-LaDoua LidarCore)
  add_test(TestCarGeometricCalibration-LaDoua
    ${TEST_BINARY_DIR}/TestCarGeometricCalibration-LaDoua
    ${data_dir}/trajectories/la_doua_dataset
  )
  
  custom_add_executable(TestGeometricCalibration-MM TestGeometricCalibration-MM.cxx)
  target_link_libraries(TestGeometricCalibration-MM LidarCore)
  add_test(TestGeometricCalibration-MM
    ${TEST_BINARY_DIR}/TestGeometricCalibration-MM
    ${data_dir}/trajectories
  )

  custom_add_executable(TestGeometricCalibration-LaDoua TestGeometricCalibration-LaDoua.cxx)
  target_link_libraries(TestGeometricCalibration-LaDoua LidarCore)
  add_test(TestGeometricCalibration-LaDoua
    ${TEST_BINARY_DIR}/TestGeometricCalibration-LaDoua
    ${data_dir}/trajectories/la_doua_dataset
  )
  
endif()

if (LIDARVIEW_BUILD_MIDHOG)
  custom_add_executable(TestMIDHOG TestMIDHOG.cxx)
  target_link_libraries(TestMIDHOG LidarCore)
  add_test(TestMIDHOG
    ${TEST_BINARY_DIR}/TestMIDHOG
  )
endif()

# OldSlam, kept for future reference
#if (ENABLE_pcl AND ENABLE_ceres AND ENABLE_nanoflann)
#  custom_add_executable(TestSlam TestSlam.cxx)
#  add_test(TestSlam
#    ${TEST_BINARY_DIR}/TestSlam
#    ${data_dir}/Slam/VLP-16_slam_test_data.pcap
#    ${data_dir}/Slam/RefSlam.vtp
#    ${CMAKE_SOURCE_DIR}/share/VLP-16.xml
#  )
#
#  custom_add_executable(TestTrajectoryReoptimization TestTrajectoryReoptimization.cxx)
#  target_link_libraries(TestTrajectoryReoptimization LidarCore)
#  add_test(TestTrajectoryReoptimization
#    ${TEST_BINARY_DIR}/TestTrajectoryReoptimization
#    ${data_dir}/trajectories/mm05_reoptimization
#  )
#endif(ENABLE_pcl AND ENABLE_ceres AND ENABLE_nanoflann)
