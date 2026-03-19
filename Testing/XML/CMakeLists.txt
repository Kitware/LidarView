set(lidarview_test_client "$<TARGET_FILE:LidarView::LidarView>")

set(tests_with_baselines
  TestAdvancedArrays.xml
  # TestCameraProjector.xml # This test requires OpenCV which is missing in the pipeline
  TestCustomClipFilter.xml
  TestExtractPointSelection.xml
  TestOpenRecentFiles.xml
  TestSaveLoadState.xml
  TestSaveLoadTrace.xml
  TestSavePcap.xml
  TestVoxelGridFilter.xml
)

if (LIDARVIEW_USE_PDAL)
  list(APPEND tests_with_baselines TestSaveLAS.xml)
endif ()

set(tests_without_baselines
  TestAboutDialog.xml
)

# As LidarView use external baseline (in separate repository) we must set the
# baseline png as DIRECT_DATA
foreach (test_baseline ${tests_with_baselines})
  get_filename_component(test_name ${test_baseline} NAME_WE)
  set("${test_name}_USES_DIRECT_DATA" ON)
endforeach ()

paraview_add_client_tests(
  PREFIX          "lv"
  CLIENT          ${lidarview_test_client}
  BASELINE_DIR    ${lidarview_test_baseline_dir}
  DATA_DIRECTORY  ${lidarview_test_data_directory_output}
  TEST_SCRIPTS    ${tests_with_baselines}
  TEST_DATA_TARGET "LidarViewData"
)

paraview_add_client_tests(
  PREFIX          "lv"
  CLIENT          ${lidarview_test_client}
  TEST_SCRIPTS    ${tests_without_baselines}
  TEST_DATA_TARGET "LidarViewData"
)
