# Add all subdirectories as Plugins
# cf. https://stackoverflow.com/questions/7787823/cmake-how-to-get-the-name-of-all-subdirectories-of-a-directory
file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
set(dirlist "")
foreach(child ${children})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
    list(APPEND dirlist ${child})
  endif()
endforeach()

foreach(subdir ${dirlist})
message(STATUS "Adding plugin: ${subdir}")
  add_subdirectory(${subdir})
endforeach()
