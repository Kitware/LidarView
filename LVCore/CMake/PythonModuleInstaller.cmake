# From inside a CMake function, it is not AFAIK possible to get the path
# to the file defining the function.
# Thus we set this variable outside the function (cannot unset it afterward).
set(python_module_install_LIST_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(python_module_install)
  cmake_parse_arguments(python_module_install "" "NAME" "FILES" ${ARGN})

  set(python_module_dir "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages")

  # Copy python files
  set(copied_python_files)
  foreach(file ${python_module_install_FILES})
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
    set(tgt "${python_module_dir}/${file}")
    set(copied_python_files ${copied_python_files} ${tgt})
    get_filename_component(tgtDir ${tgt} PATH)
    file(MAKE_DIRECTORY ${tgtDir})
    add_custom_command(DEPENDS ${src}
                       COMMAND ${CMAKE_COMMAND} -E copy ${src} ${tgt}
                       OUTPUT ${tgt}
                       COMMENT "source copy")
  endforeach(file)

  # Byte compile python files
  set(compile_all_script "${CMAKE_CURRENT_BINARY_DIR}/compile_all.py")

  configure_file("${python_module_install_LIST_DIR}/PythonModuleInstaller_compile_all.py.in"
                 ${compile_all_script}
                 @ONLY IMMEDIATE)

  add_custom_command(
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS  "${compile_all_script}"
    DEPENDS ${copied_python_files}  ${compile_all_script}
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/compile_complete"
    )

  add_custom_target("${python_module_install_NAME}-python-module" ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/compile_complete")


  # Install python module
  set(install_destination ${VV_INSTALL_LIBRARY_DIR}/site-packages)
  if (APPLE)
    set(install_destination ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Python)
  endif()

  install(DIRECTORY  ${python_module_dir}/${python_module_install_NAME}
          DESTINATION ${install_destination}
          COMPONENT Runtime
          USE_SOURCE_PERMISSIONS)
endfunction()
