set(lidarview_test_client "$<TARGET_FILE:${SOFTWARE_TARGET}>")

set(tests_with_baselines
  TestUICropAndExportPythonExample.xml
  TestUILoadLidarDataPythonExample.xml
  # TestUIPlaneFitAndProcessPythonExample.xml
  TestUIResetWindowViewsPythonExample.xml
  # TestUISaveSelectedPointOnMultipleFramePythonExample.xml
  # TestUISplitWindowPythonExample.xml
)

paraview_add_client_tests(
  PREFIX          "lv"
  CLIENT          ${lidarview_test_client}
  BASELINE_DIR    ${lidarview_test_baseline_dir}
  DATA_DIRECTORY  ${lidarview_test_data_directory_output}
  TEST_SCRIPTS    ${tests_with_baselines}
  TEST_DATA_TARGET "LidarViewData"
)
