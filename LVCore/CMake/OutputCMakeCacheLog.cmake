message(STATUS "Output CMakeCacheLog.txt")
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
#message("VARS=${_variableNames}=")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -N -LA
  OUTPUT_VARIABLE out
  RESULT_VARIABLE res
  ERROR_VARIABLE  err)
#message(STATUS "OUT=${out}=")
if (res)
  message(FATAL_ERROR "Failed to dump CMakeCacheLog.txt:\n${err}")
endif ()

file(WRITE "CMakeCacheLog.txt" "${out}" )
file(INSTALL "CMakeCacheLog.txt" DESTINATION "${CMAKE_INSTALL_PREFIX}/share" )
