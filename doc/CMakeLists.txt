find_package(Sphinx REQUIRED)

message("TEST: ${SPHINX_EXECUTABLE}")

set(tutorial_dir "${CMAKE_BINARY_DIR}/doc/tutorial")
add_custom_command(
  OUTPUT "${tutorial_dir}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${tutorial_dir}"
)

add_custom_target(tutorials ALL
  COMMAND ${SPHINX_EXECUTABLE}
    -b html
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${tutorial_dir}
  DEPENDS ${tutorial_dir}
  )

install(DIRECTORY "${CMAKE_BINARY_DIR}/doc/tutorial"
        DESTINATION "${LV_INSTALL_RUNTIME_DIR}")

set_property(
  TARGET tutorials
  PROPERTY ADDITIONAL_CLEAN_FILES ${tutorial_dir}
  )
