find_package(Sphinx REQUIRED)

set(documentation_dir "${CMAKE_BINARY_DIR}/doc")
add_custom_command(
  OUTPUT "${tutorial_dir}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${documentation_dir}"
)

add_custom_target(documentation ALL
  COMMAND ${SPHINX_EXECUTABLE}
    -b html
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${documentation_dir}
  DEPENDS ${documentation_dir}
  )

set_property(
  TARGET documentation
  PROPERTY ADDITIONAL_CLEAN_FILES ${documentation_dir}
  )
