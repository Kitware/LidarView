#[==[.md
## Fetch LidarView test data with git lfs.

Arguments:
  * `DATA_NAME`: (Required) Target data name.
  * `COMMIT_SHA`: (Optionnal) If specified checkout data on this sha, otherwise
    checkout on `DATA_NAME` branch.

Variable set:
  * `FETCH_TEST_DATA_DIR`: Path to test data directory.
#]==]
function (lidarview_fetch_test_data)
  cmake_parse_arguments(_lidarview_fetch_test_data
    ""
    "DATA_NAME;COMMIT_SHA"
    ""
    ${ARGN}
  )

  if (NOT DEFINED _lidarview_fetch_test_data_DATA_NAME)
    message(FATAL_ERROR "A valid name must be set.")
  endif ()

  set(git_tag "${_lidarview_fetch_test_data_DATA_NAME}-data")
  if (DEFINED _lidarview_fetch_test_data_COMMIT_SHA)
    set(git_tag ${_lidarview_fetch_test_data_COMMIT_SHA})
  endif ()

  include(FetchContent)
  set(fetch_content_name "${_lidarview_fetch_test_data_DATA_NAME}-data")
  FetchContent_Declare(${fetch_content_name}
      GIT_REPOSITORY "https://gitlab.kitware.com/LidarView/lidarview-test-data.git"
      GIT_TAG        ${git_tag}
      SOURCE_DIR     "${CMAKE_BINARY_DIR}/_data/${fetch_content_name}"
      BINARY_DIR     "${CMAKE_BINARY_DIR}/_data/.${fetch_content_name}-cache/build"
      SUBBUILD_DIR   "${CMAKE_BINARY_DIR}/_data/.${fetch_content_name}-cache/subbuild"
  )
  FetchContent_GetProperties(${fetch_content_name})
  if (NOT ${fetch_content_name}_POPULATED)
      message(STATUS "Fetching ${fetch_content_name}")
      FetchContent_Populate(${fetch_content_name})
  endif ()

  set(data_folder "${${fetch_content_name}_SOURCE_DIR}/${_lidarview_fetch_test_data_DATA_NAME}")
  if (NOT EXISTS "${data_folder}")
    message(FATAL_ERROR "${data_folder} does not exist.")
  endif ()

  set(FETCH_TEST_DATA_DIR "${data_folder}" PARENT_SCOPE)
endfunction ()
