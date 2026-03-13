find_package(Doxygen REQUIRED)

set(DOCUMENTATION_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Documentation/")

set(doxygen_in ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in)
set(doxygen_out ${CMAKE_CURRENT_BINARY_DIR}/doxyfile)

# request to configure the file
configure_file(${doxygen_in} ${doxygen_out} @ONLY)

message(STATUS "Doxygen build started")

add_custom_target(doc_doxygen ALL
  COMMAND ${DOXYGEN_EXECUTABLE} ${doxygen_out}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating API documentation with Doxygen"
  VERBATIM)
