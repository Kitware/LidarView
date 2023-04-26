#[==[.md
## Install python module

Inspired from paraview `Wrapping/Python/CMakeLists.txt`

  * `NAME`: (Required) Target name.
  * `FILES`: (Required) Python files to be installed.
  * `OUTPUT_DIRECTORY`: (Optional) Install FILES in OUTPUT_DIRECTORY.
                        Note that no __init__.py are created during this process.
#]==]
function (python_module_install)
  cmake_parse_arguments(_python_module_install
    ""
    "NAME;OUTPUT_DIRECTORY"
    "FILES"
    ${ARGN})

  if (_python_module_install_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for _python_module_install: "
      "${_python_module_install_UNPARSED_ARGUMENTS}")
  endif ()
  if (NOT DEFINED _python_module_install_NAME OR NOT DEFINED _python_module_install_FILES)
    message(FATAL_ERROR "`NAME` and `FILES` arguments are required.")
  endif ()

  set(_output_directory_basepath "${CMAKE_BINARY_DIR}/${LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX}")
  if (DEFINED _python_module_install_OUTPUT_DIRECTORY)
    set(_output_directory_basepath "${_output_directory_basepath}/${_python_module_install_OUTPUT_DIRECTORY}")
  endif ()

  set(_python_copied_modules)
  foreach (_python_file IN LISTS _python_module_install_FILES)
    set(_output_python_file "${_output_directory_basepath}/${_python_file}")
    if (_python_file MATCHES "\\.in$")
      string(REPLACE ".in" "" _output_python_file "${_output_python_file}")
      configure_file(
        "${_python_file}"
        "${_output_python_file}"
        @ONLY)
    else ()
      add_custom_command(
        OUTPUT  "${_output_python_file}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_python_file}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/${_python_file}"
                "${_output_python_file}"
        COMMENT "Copying ${CMAKE_CURRENT_SOURCE_DIR}/${_python_file} to ${_output_python_file} binary directory")
    endif ()

    set(_python_directory)
    cmake_path(GET _python_file PARENT_PATH _python_directory)

    if (DEFINED _python_module_install_OUTPUT_DIRECTORY)
      set(_python_directory "${_python_directory}/${_python_module_install_OUTPUT_DIRECTORY}")
    endif ()

    install(
      FILES       "${_output_python_file}"
      DESTINATION "${LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX}/${_python_directory}"
      COMPONENT   "python")
    list(APPEND _python_copied_modules
      "${_output_python_file}")
  endforeach ()

  add_custom_target(lidarview_python_install_${_python_module_install_NAME} ALL
    DEPENDS
      ${_python_copied_modules})
endfunction ()

