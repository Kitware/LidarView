# Set these to make the testing macros work properly.
set(_vtk_build_TEST_INPUT_DATA_DIRECTORY "${lidarview_test_data_directory_input}")
set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY "${lidarview_test_data_directory_output}")
set(_vtk_build_TEST_OUTPUT_DIRECTORY "${lidarview_test_output_directory}")
set(lidarview_test_baseline_dir "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline")

if (LIDARVIEW_USE_PYTHON)
    add_subdirectory(Python)
endif ()

if (SOFTWARE_TARGET AND LIDARVIEW_USE_QT)
    add_subdirectory(XML)
endif ()
