# File inspired from vtkModuleTesting.cmake

#[==[.rst:
If the ``_vtk_testing_python_exe`` variable is not set, the ``lvpython`` binary is
used by default. Additional arguments may be passed in this variable as well.

If the given Python executable supports VTK's ``--enable-bt`` flag, setting
``_vtk_testing_python_exe_supports_bt`` to ``1`` is required to enable it.
#]==]

#[==[.rst
Options:

- ``NO_DATA``
- ``NO_VALID``
- ``NO_OUTPUT``
- ``NO_RT``
- ``JUST_VALID``
- ``LEGACY_VALID``
- ``TIGHT_VALID``
- ``LOOSE_VALID``

Each argument should be either an option, a test specification, or it is passed
as flags to all tests declared in the group. The list of tests is set in the
``<VARNAME>`` variable in the calling scope.

Options:

- ``NO_DATA``: The test does not need to know the test input data directory. If
  it does, it is passed on the command line via the ``-D`` flag.
- ``NO_OUTPUT``: The test does not need to write out any data to the
  filesystem. If it does, a directory which may be written to is passed via
  the ``-T`` flag.
- ``NO_VALID``: The test does not have a valid baseline image. If it does, the
  baseline is assumed to be in ``../Data/Baseline/<NAME>.png`` relative to the
  current source directory. If alternate baseline images are required,
  ``<NAME>`` may be suffixed by ``_1``, ``_2``, etc. The valid image is passed via
  the ``-V`` flag.
- ``NO_RT``: If ``NO_RT`` is specified, ``-B`` is passed instead of ``-V``, only
   providing a baseline dir, assuming ``NO_VALID`` is not specified.
- ``JUST_VALID``: Only applies when neither ``NO_VALID`` or ``NO_RT`` are present.
  If it is not specified, the test is run via ``vtkmodules.test.rtImageTest``.
- ``TIGHT_VALID``: Default behavior if legacy image comparison method is turned off by default.
  The baseline is tested using an euclidean metric, which is sensitive to outliers.
- ``LOOSE_VALID``: The baseline is tested using an norm-1 metric, which is less sensitive to
  outliers. It should typically be used when comparing text or when testing rendering that
  varies a lot depending on the GPU drivers.
- ``LEGACY_VALID``: Uses legacy image compare metric, which is more forgiving than the new one.

Additional flags may be passed to tests using the ``${_vtk_build_test}_ARGS``
variable or the ``<NAME>_ARGS`` variable.
To use a different baseline name than ``<NAME>`` you can set
``<NAME>_BASELINE_NAME`` variable. This is the name of the image file without extension.
#]==]
function (lidarview_add_test_python)
  if (NOT _vtk_testing_python_exe)
    set(_vtk_testing_python_exe "$<TARGET_FILE:LidarView::lvpython>")
    set(_vtk_testing_python_exe_supports_bt 1)
  endif ()
  set(python_options
    NO_DATA
    NO_VALID
    NO_OUTPUT
    NO_RT
    JUST_VALID
    LEGACY_VALID
    TIGHT_VALID
    LOOSE_VALID
    )
  _vtk_test_parse_args("${python_options}" "py" ${ARGN})
  _vtk_test_set_options("${python_options}" "" ${options})
  if (NOT DEFINED lidarview_test_baseline_dir)
    message(FATAL_ERROR "Please set 'lidarview_test_baseline_dir' first.")
  endif ()

  set(_vtk_fail_regex "(\n|^)ERROR: " "ERR\\|" "instance(s)? still around|DeprecationWarning")

  set(_vtk_skip_regex
    # Insufficient graphics resources.
    "Attempt to use a texture buffer exceeding your hardware's limits"
    # Vulkan driver not setup correctly.
    "vulkan: No DRI3 support detected - required for presentation")

  foreach (name IN LISTS names)
    _vtk_test_set_options("${python_options}" "local_" ${_${name}_options})
    _vtk_test_parse_name(${name} "py")

    set(_D "")
    if (NOT local_NO_DATA)
      set(_D -D "${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}")
    endif ()

    set(rtImageTest "")
    set(_B "")
    set(_V "")
    set(image_compare_method ${default_image_compare})
    if (NOT local_NO_VALID)
      if (DEFINED "${test_name}_BASELINE_NAME")
        set(baseline_name "${${test_name}_BASELINE_NAME}")
      else()
        set(baseline_name "${test_name}")
      endif()
      if (local_NO_RT)
        set(_B -B "${lidarview_test_baseline_dir}/")
      else ()
        set(_V -V "${lidarview_test_baseline_dir}/${baseline_name}.png")
        if (NOT local_JUST_VALID)
          set(rtImageTest -m "vtkmodules.test.rtImageTest")
        endif ()
      endif ()
      if (local_LEGACY_VALID)
        set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=LEGACY_VALID")
      elseif (local_TIGHT_VALID)
        set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
      elseif (local_LOOSE_VALID)
        set(image_compare_method ";VTK_TESTING_IMAGE_COMPARE_METHOD=LOOSE_VALID")
      endif ()
    endif ()

    set(_T "")
    if (NOT local_NO_OUTPUT)
      set(_T -T "${_vtk_build_TEST_OUTPUT_DIRECTORY}")
    endif ()

    if (NOT _vtk_build_TEST_FILE_DIRECTORY)
      set(_vtk_build_TEST_FILE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if (VTK_USE_MPI AND
        VTK_SERIAL_TESTS_USE_MPIEXEC AND
        NOT DEFINED _vtk_test_python_pre_args)
      set(_vtk_test_python_pre_args
        "${MPIEXEC_EXECUTABLE}"
        "${MPIEXEC_NUMPROC_FLAG}" "1"
        ${MPIEXEC_PREFLAGS})
    endif()
    set(_vtk_test_python_bt_args)
    if (_vtk_testing_python_exe_supports_bt)
      list(APPEND _vtk_test_python_bt_args --enable-bt)
    endif ()
    set(testArgs NAME "${_vtk_build_test}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}"
                 COMMAND ${_vtk_test_python_pre_args}
                         "${_vtk_testing_python_exe}" ${_vtk_test_python_args}
                         ${_vtk_test_python_bt_args}
                         ${rtImageTest}
                         "${_vtk_build_TEST_FILE_DIRECTORY}/${test_file}"
                         ${args}
                         ${${_vtk_build_test}_ARGS}
                         ${${test_name}_ARGS}
                         ${_D} ${_B} ${_T} ${_V})

    add_test(${testArgs})

    if (_vtk_testing_ld_preload)
      set_property(TEST "${_vtk_build_test}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}"
        APPEND
        PROPERTY
          ENVIRONMENT "LD_PRELOAD=${_vtk_testing_ld_preload}")
    endif ()

    set_tests_properties("${_vtk_build_test}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}"
      PROPERTIES
        LABELS "${_vtk_build_test_labels}"
        FAIL_REGULAR_EXPRESSION "${_vtk_fail_regex}"
        SKIP_REGULAR_EXPRESSION "${_vtk_skip_regex}"
        ENVIRONMENT "VTK_TESTING=1;${image_compare_method}"
        # This must match the skip() function in vtk/test/Testing.py"
        SKIP_RETURN_CODE 125
      )

    if (numprocs)
      set_tests_properties("${_vtk_build_test}Python${_vtk_test_python_suffix}-${vtk_test_prefix}${test_name}"
        PROPERTIES
          PROCESSORS "${numprocs}")
    endif ()
  endforeach ()
endfunction ()
