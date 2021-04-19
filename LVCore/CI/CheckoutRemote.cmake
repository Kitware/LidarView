#CheckoutRemote.cmake
#Used by CI using cmake -DCI_DEPLOY_PASSWORD=$CI_DEPLOY_PASSWORD -DTRIGGER_MODULE_SHA=$TRIGGER_MODULE_SHA -DTRIGGER_MODULE_REMOTE_NAME=$TRIGGER_MODULE_REMOTE_NAME -DTRIGGER_MODULE_REPOSITORY_PATH=$TRIGGER_MODULE_REPOSITORY_PATH -P CI/CheckoutRemote.cmake

#Stop if no remote checking is required
if(NOT TRIGGER_MODULE_SHA OR NOT TRIGGER_MODULE_REMOTE_NAME OR NOT TRIGGER_MODULE_REPOSITORY_PATH)
  message(STATUS "No Specific Remote requested for build.")
  return() #exit script
else()
  message(STATUS "Requested checking out ${TRIGGER_MODULE_SHA} from ${TRIGGER_MODULE_REMOTE_NAME} in ${TRIGGER_MODULE_REPOSITORY_PATH}")
endif()

Find_package(Git QUIET)
if(NOT GIT_EXECUTABLE)
  message(FATAL_ERROR "error: could not find git")
endif()

execute_process(
  COMMAND ${GIT_EXECUTABLE} remote add ${TRIGGER_MODULE_REMOTE_NAME} https://gitlab-ci-token:${CI_DEPLOY_PASSWORD}@${TRIGGER_MODULE_REPOSITORY_PATH}
  WORKING_DIRECTORY ${TRIGGER_MODULE_REPOSITORY_PATH}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE  error
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
  ECHO_OUTPUT_VARIABLE
  ECHO_ERROR_VARIABLE
)
if(result)
  string(FIND output "already exists" find_alreadyexists) #Adding the same remote is okay
  if(NOT find_alreadyexists)
      message(FATAL_ERROR "Adding remote failed: ${output} ${error}")
  endif()
endif()


