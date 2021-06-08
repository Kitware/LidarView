# Sanitize checks
if(NOT LV_INSTALL_LIBRARY_DIR )
  message(FATAL_ERROR "LV_INSTALL_LIBRARY_DIR not set")
endif()

if (WIN32)
  get_filename_component(INSTALL_LOCAL_DIR "${CMAKE_INSTALL_PREFIX}/bin" ABSOLUTE)
else()
  set(INSTALL_LOCAL_DIR "${CMAKE_BINARY_DIR}/bin")
endif()

set(data_dir ${CMAKE_CURRENT_SOURCE_DIR}/Data)

function(custom_add_executable)
  add_executable(${ARGV})
  if (WIN32)
    install(TARGETS ${ARGV0}
	    RUNTIME DESTINATION ${INSTALL_LOCAL_DIR}
	    LIBRARY DESTINATION ${LV_INSTALL_LIBRARY_DIR}
	    COMPONENT Runtime)
  endif(WIN32)
endfunction(custom_add_executable)

# Add TestHelpers
target_sources(LidarPlugin PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/TestHelpers.cxx
  )
target_include_directories(LidarPlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Default Tests
custom_add_executable(TestNMEAParser TestNMEAParser.cxx)
target_link_libraries(TestNMEAParser LidarPlugin)

custom_add_executable(TestTrailingFrame TestTrailingFrame.cxx)
target_link_libraries(TestTrailingFrame LidarPlugin)

custom_add_executable(TestRansacPlaneModel TestRansacPlaneModel.cxx)
target_link_libraries(TestRansacPlaneModel LidarPlugin)

#custom_add_executable(TestVtkEigenTools TestVtkEigenTools.cxx )
#target_link_libraries(TestVtkEigenTools LidarPlugin)
#add_test(TestVtkEigenTools
#  ${INSTALL_LOCAL_DIR}/TestVtkEigenTools
#)

custom_add_executable(TestBoundingBox TestBoundingBox.cxx)
target_link_libraries(TestBoundingBox LidarPlugin)

custom_add_executable(TestTemporalTransformsReaderWriter TestTemporalTransformsReaderWriter.cxx )
target_link_libraries(TestTemporalTransformsReaderWriter LidarPlugin)

add_test(TestNMEAParser
  ${INSTALL_LOCAL_DIR}/TestNMEAParser
)

add_test(TestTrailingFrame
  ${INSTALL_LOCAL_DIR}/TestTrailingFrame
)

add_test(TestRansacPlaneModel
  ${INSTALL_LOCAL_DIR}/TestRansacPlaneModel
)

custom_add_executable(TestCarGeometricCalibration-LaDoua TestCarGeometricCalibration-LaDoua.cxx)
target_link_libraries(TestCarGeometricCalibration-LaDoua LidarPlugin)
add_test(TestCarGeometricCalibration-LaDoua
  ${INSTALL_LOCAL_DIR}/TestCarGeometricCalibration-LaDoua
  ${data_dir}/trajectories/la_doua_dataset
)

custom_add_executable(TestCarGeometricCalibration-MM TestCarGeometricCalibration-MM.cxx)
target_link_libraries(TestCarGeometricCalibration-MM LidarPlugin)
add_test(TestCarGeometricCalibration-MM
  ${INSTALL_LOCAL_DIR}/TestCarGeometricCalibration-MM
  ${data_dir}/trajectories
)

add_test(TestTemporalTransformsReaderWriter
  ${INSTALL_LOCAL_DIR}/TestTemporalTransformsReaderWriter
  ${data_dir}/trajectories/mm04/orbslam2-no-loop-closure.csv
  ${data_dir}/trajectories/mm04/orbslam2-no-loop-closure.csv.temporary
)

add_test(TestBoundingBox
  ${INSTALL_LOCAL_DIR}/TestBoundingBox
)

#custom_add_executable(TestScaleCalibration-MM TestScaleCalibration-MM.cxx)
#target_link_libraries(TestScaleCalibration-MM LidarPlugin)
#add_test(TestScaleCalibration-MM
#  ${INSTALL_LOCAL_DIR}/TestScaleCalibration-MM
#  ${data_dir}/trajectories
#)

# Comment this test are they randomly fail on osx (error and even segfault)
#custom_add_executable(TestTimeCalibration-LaDoua TestTimeCalibration-LaDoua.cxx)
#target_link_libraries(TestTimeCalibration-LaDoua LidarPlugin)
#add_test(TestTimeCalibration-LaDoua
#  ${INSTALL_LOCAL_DIR}/TestTimeCalibration-LaDoua
#  ${data_dir}/trajectories/la_doua_dataset
#)

#custom_add_executable(TestTimeCalibration-MM TestTimeCalibration-MM.cxx)
#target_link_libraries(TestTimeCalibration-MM LidarPlugin)

#add_test(TestTimeCalibration-MM
#  ${INSTALL_LOCAL_DIR}/TestTimeCalibration-MM
#  ${data_dir}/trajectories
#)

# Optional Tests
if(LIDARVIEW_BUILD_CALIBFROMPOSES)
  custom_add_executable(TestCameraCalibration TestCameraCalibration.cxx)
  target_link_libraries(TestCameraCalibration LidarPlugin)
  add_test(TestCameraCalibration
    ${INSTALL_LOCAL_DIR}/TestCameraCalibration
    ${data_dir}/Camera/MatchedPoints_3D_2D
  )

  custom_add_executable(TestGeometricCalibration-MM TestGeometricCalibration-MM.cxx)
  target_link_libraries(TestGeometricCalibration-MM LidarPlugin)
  add_test(TestGeometricCalibration-MM
    ${INSTALL_LOCAL_DIR}/TestGeometricCalibration-MM
    ${data_dir}/trajectories
  )

  custom_add_executable(TestGeometricCalibration-LaDoua TestGeometricCalibration-LaDoua.cxx)
  target_link_libraries(TestGeometricCalibration-LaDoua LidarPlugin)
  add_test(TestGeometricCalibration-LaDoua
    ${INSTALL_LOCAL_DIR}/TestGeometricCalibration-LaDoua
    ${data_dir}/trajectories/la_doua_dataset
  )

endif()

if (LIDARVIEW_BUILD_MIDHOG)
  custom_add_executable(TestMIDHOG TestMIDHOG.cxx)
  target_link_libraries(TestMIDHOG LidarPlugin)
  add_test(TestMIDHOG
    ${INSTALL_LOCAL_DIR}/TestMIDHOG
  )
endif()

# OldSlam, kept for future reference
#if (ENABLE_pcl AND ENABLE_ceres AND ENABLE_nanoflann)
#  add_test(TestSlam
#    ${INSTALL_LOCAL_DIR}/TestSlam
#    ${data_dir}/Slam/VLP-16_slam_test_data.pcap
#    ${data_dir}/Slam/RefSlam.vtp
#    ${CMAKE_SOURCE_DIR}/share/VLP-16.xml
#  )
#
#  add_executable(TestTrajectoryReoptimization TestTrajectoryReoptimization.cxx)
#  target_link_libraries(TestTrajectoryReoptimization LidarPlugin)
#  add_test(TestTrajectoryReoptimization
#    ${INSTALL_LOCAL_DIR}/TestTrajectoryReoptimization
#    ${data_dir}/trajectories/mm05_reoptimization
#  )
#endif(ENABLE_pcl AND ENABLE_ceres AND ENABLE_nanoflann)
