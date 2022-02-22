# Enable Check
if(NOT CI_VELODYNEPRIVATE_ENABLE)
  message(STATUS "VelodynePrivate Remote Addition not requested")
  return()
else()
  message(STATUS "VelodynePrivate Remote Addition requested")
endif()

# Sanity Checks
if(NOT CI_VELODYNEPRIVATE_USER)
  message(FATAL_ERROR "CI_VELODYNEPRIVATE_USER not set")
  return()
endif()
if(NOT CI_VELODYNEPRIVATE_TOKEN)
  message(FATAL_ERROR "CI_VELODYNEPRIVATE_TOKEN not set")
  return()
endif()

Find_package(Git QUIET)
if(NOT GIT_EXECUTABLE)
  message(FATAL_ERROR "error: could not find git")
endif()

# Set Variables
SET(VV_REMOTE_URL "https://${CI_VELODYNEPRIVATE_USER}:${CI_VELODYNEPRIVATE_TOKEN}@gitlab.kitware.com/LidarView/velodyneplugin-private.git")
SET(VV_PLUGIN_PATH "${CMAKE_CURRENT_LIST_DIR}/../../Plugins/VelodynePlugin")

# Exec Git Command
execute_process(
  COMMAND ${GIT_EXECUTABLE} -C ${VV_PLUGIN_PATH} remote add private ${VV_REMOTE_URL}
  COMMAND ${GIT_EXECUTABLE} -C ${VV_PLUGIN_PATH} remote -vv
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE  error
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
)
message(STATUS "output: ${output}") #CMake 3.18+ feature
message(STATUS "error:  ${error}")
if(result)
  string(FIND output "already exists" find_alreadyexists) #Adding the same remote is okay
  if(NOT find_alreadyexists)
      message(FATAL_ERROR "Adding remote failed")
  endif()
endif()
