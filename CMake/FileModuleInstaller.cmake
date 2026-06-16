#[==[.md
## Install file module

Inspired from paraview `Wrapping/Python/CMakeLists.txt`

  * `NAME`: (Required) Target name.
  * `FILES`: (Required) Files to be installed.
  * `INSTALL_DIRECTORY`: (Required) Where to install files.
  * `OUTPUT_DIRECTORY`: (Optional) Install FILES in OUTPUT_DIRECTORY.
                        Note that no __init__.py are created during this process.
#]==]
function (file_module_install)
  cmake_parse_arguments(_file_module_install
    ""
    "NAME;OUTPUT_DIRECTORY;INSTALL_DIRECTORY"
    "FILES"
    ${ARGN})

  if (_file_module_install_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for _file_module_install: "
      "${_file_module_install_UNPARSED_ARGUMENTS}")
  endif ()
  if (NOT DEFINED _file_module_install_NAME OR NOT DEFINED _file_module_install_FILES OR NOT DEFINED _file_module_install_INSTALL_DIRECTORY)
    message(FATAL_ERROR "`NAME`, `FILES` and `INSTALL_DIRECTORY` arguments are required.")
  endif ()

  set(_output_directory_basepath "${CMAKE_BINARY_DIR}/${_file_module_install_INSTALL_DIRECTORY}")
  if (DEFINED _file_module_install_OUTPUT_DIRECTORY)
    set(_output_directory_basepath "${_output_directory_basepath}/${_file_module_install_OUTPUT_DIRECTORY}")
  endif ()

  set(_file_copied_modules)
  foreach (_file_name IN LISTS _file_module_install_FILES)
    get_filename_component(_file_basename "${_file_name}" NAME)
    set(_output_file "${_output_directory_basepath}/${_file_basename}")
    if (_file_name MATCHES "\\.in$")
      string(REPLACE ".in" "" _output_file "${_output_file}")
      configure_file(
        "${_file_name}"
        "${_output_file}"
        @ONLY)
    else ()
      add_custom_command(
        OUTPUT  "${_output_file}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_file_name}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/${_file_name}"
                "${_output_file}"
        COMMENT "Copying ${_file_name} to ${_output_file} binary directory")
    endif ()

    set(_file_directory)
    if (DEFINED _file_module_install_OUTPUT_DIRECTORY)
      set(_file_directory "${_file_module_install_OUTPUT_DIRECTORY}")
    endif ()

    install(
      FILES       "${_output_file}"
      DESTINATION "${_file_module_install_INSTALL_DIRECTORY}/${_file_directory}"
      COMPONENT   "lidarview_install_files")
    list(APPEND _file_copied_modules
      "${_output_file}")
  endforeach ()

  add_custom_target(lidarview_install_${_file_module_install_NAME} ALL
    DEPENDS
      ${_file_copied_modules})
endfunction ()

#[==[.md
## Install python module

  * `NAME`: (Required) Target name.
  * `FILES`: (Required) Python files to be installed.
  * `OUTPUT_DIRECTORY`: (Optional) Install FILES in OUTPUT_DIRECTORY.
                        Note that no __init__.py are created during this process.
#]==]
function (python_module_install)
  file_module_install(
    INSTALL_DIRECTORY "${LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX}"
    ${ARGN}
  )
endfunction ()

