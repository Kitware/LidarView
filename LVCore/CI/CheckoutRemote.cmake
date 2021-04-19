#CheckoutRemote.cmake
#Used by CI using cmake -DCI_DEPLOY_USER=$CI_DEPLOY_USER -DCI_DEPLOY_PASSWORD=$CI_DEPLOY_PASSWORD -DTRIGGER_MODULE_SHA=$TRIGGER_MODULE_SHA -DTRIGGER_MODULE_REMOTE_NAME=$TRIGGER_MODULE_REMOTE_NAME -DTRIGGER_MODULE_REPOSITORY_PATH=$TRIGGER_MODULE_REPOSITORY_PATH -P CI/CheckoutRemote.cmake

#Stop if no remote checking is required
if(NOT TRIGGER_MODULE_SHA OR NOT TRIGGER_MODULE_REMOTE_NAME OR NOT TRIGGER_MODULE_REPOSITORY_PATH)
  message(STATUS "No Specific Remote requested for build.")
  return() #exit script
endif()

if(NOT CI_DEPLOY_USER OR NOT CI_DEPLOY_PASSWORD)
  message(FATAL_ERROR "CI RUNNER USER and PASSWORD required")
endif()

Find_package(Git QUIET)
if(NOT GIT_EXECUTABLE)
  message(FATAL_ERROR "error: could not find git")
endif()

set(NEW_REMOTE "https://${CI_DEPLOY_USER}:${CI_DEPLOY_PASSWORD}@${TRIGGER_MODULE_REPOSITORY_PATH}" )
message(STATUS "Requested checking out ${TRIGGER_MODULE_SHA} from ${TRIGGER_MODULE_REMOTE_NAME} in ${TRIGGER_MODULE_REPOSITORY_PATH}")
message(STATUS "Full link: ${NEW_REMOTE}")

execute_process(
  COMMAND ${GIT_EXECUTABLE} remote add ${TRIGGER_MODULE_REMOTE_NAME} ${NEW_REMOTE}
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


