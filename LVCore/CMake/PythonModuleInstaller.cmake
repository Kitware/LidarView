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
  set(module_install_dir "${CMAKE_INSTALL_PREFIX}/${LV_INSTALL_PYTHON_MODULES_DIR}/${python_module_install_NAME}")

  # Compile Module in-place
  add_custom_target("Python-${python_module_install_NAME}" ALL
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${module_install_dir}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${python_module_install_PATH} ${module_install_dir}
    COMMAND ${Python3_EXECUTABLE} -c "import compileall;compileall.compile_dir('${module_install_dir}')"
    COMMENT "Python-${python_module_install_NAME}"
    VERBATIM
  )
endfunction()
