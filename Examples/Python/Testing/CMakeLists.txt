# Set these to make the testing macros work properly.
set(_vtk_build_TEST_INPUT_DATA_DIRECTORY "${lidarview_test_data_directory_input}")
set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY "${lidarview_test_data_directory_output}")
set(_vtk_build_TEST_OUTPUT_DIRECTORY "${lidarview_test_output_directory}")

if (LIDARVIEW_USE_QT)
  add_subdirectory(XML)
endif ()
