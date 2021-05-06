if(NOT LV_INSTALL_PYTHON_MODULES_DIR)
  message(FATAL_ERROR "LV_INSTALL_PYTHON_MODULES_DIR not set")
endif()

include(CMakeParseArguments)

function(python_module_install)
  cmake_parse_arguments(python_module_install "" "NAME;PATH" "FILES" ${ARGN})

  if(NOT python_module_install_NAME)
    message(FATAL_ERROR "python_module_install requires NAME argument")
  endif()

  if(NOT python_module_install_PATH)
    # Assume the Path is the directly python_module_install_NAME
    set(python_module_install_PATH ${python_module_install_NAME})
  endif()

  if(python_module_install_FILES)
    message(DEPRECATION "python_module_install FILES is deprecated, you must bundle into a python module and supply its NAME")
    message(FATAL_ERROR "Stopping...")
  endif()

  # Compilation directory
  set(python_module_dir "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages")
  set(module_compile_dir "${python_module_dir}/${python_module_install_NAME}")
  set(module_install_dir "${LV_INSTALL_PYTHON_MODULES_DIR}/${python_module_install_NAME}")

  # Set a Custom Command to do at the end
  # WIP could have been done right away with execute process, but unsure about DEPENDS
  add_custom_target("Python-module-${python_module_install_NAME}" ALL
    # Move to Compil dir
    COMMAND ${CMAKE_COMMAND} -E echo "Getting module at ${python_module_install_PATH}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${python_module_install_PATH} ${module_compile_dir}
    # Compile module
    COMMAND ${CMAKE_COMMAND} -E echo "Compiling ${module_compile_dir}"
    COMMAND ${Python3_EXECUTABLE} -c "import compileall;compileall.compile_dir('${module_compile_dir}')"
    # Clean and Install compiled module
    COMMAND ${CMAKE_COMMAND} -E echo "Installing to ${CMAKE_INSTALL_PREFIX}/${module_install_dir}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${module_install_dir}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${module_compile_dir} ${CMAKE_INSTALL_PREFIX}/${module_install_dir}
    DEPENDS ${copied_python_files}
    COMMENT "Python-module-${python_module_install_NAME}"
    VERBATIM
    USES_TERMINAL
  )

endfunction()
