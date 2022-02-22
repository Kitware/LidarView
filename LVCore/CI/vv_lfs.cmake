# Enable Check
if(NOT CI_VELODYNEPRIVATE_ENABLE)
  message(STATUS "VelodynePrivate LFS Fetch not requested")
  return()
else()
  message(STATUS "VelodynePrivate LFS Fetch requested")
endif()

Find_package(Git QUIET)
if(NOT GIT_EXECUTABLE)
  message(FATAL_ERROR "error: could not find git")
endif()

# Set Variables
SET(VV_PLUGIN_PATH "${CMAKE_CURRENT_LIST_DIR}/../../Plugins/VelodynePlugin")

# Exec Git Command
execute_process(
  COMMAND ${GIT_EXECUTABLE} -C ${VV_PLUGIN_PATH} lfs fetch private --prune
  COMMAND ${GIT_EXECUTABLE} -C ${VV_PLUGIN_PATH} lfs fetch origin --prune
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE  error
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
)
message(STATUS "output: ${output}") #CMake 3.18+ feature
message(STATUS "error:  ${error}")
