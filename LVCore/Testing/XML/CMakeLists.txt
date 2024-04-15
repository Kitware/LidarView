set(lidarview_test_client "$<TARGET_FILE:${SOFTWARE_TARGET}>")

set(tests_with_baselines
  TestAdvancedArrays.xml
  # TestCameraProjector.xml # This test requires OpenCV which is missing in the pipeline
  TestCustomClipFilter.xml
  TestExtractPointSelection.xml
  TestOpenRecentFiles.xml
  TestSaveLAS.xml
  TestSaveLoadState.xml
  TestSaveLoadTrace.xml
  TestSavePcap.xml
  TestVoxelGridFilter.xml
)

set(tests_without_baselines
  TestAboutDialog.xml
)

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
