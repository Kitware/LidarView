# Add all subdirectories as Plugins

function(list_directories curdir result)
  # cf. https://stackoverflow.com/questions/7787823/cmake-how-to-get-the-name-of-all-subdirectories-of-a-directory
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(tmp_result "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND tmp_result ${child})
    endif()
  endforeach()
set(${result} ${tmp_result} PARENT_SCOPE)
endfunction()

list_directories(${CMAKE_CURRENT_SOURCE_DIR} dirlist)

foreach(subdir ${dirlist})
message(STATUS "Adding plugin: ${subdir}")
  add_subdirectory(${subdir})
endforeach()
