function (lidarview_add_executable name)
  # Set up rpaths
  set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
  if (UNIX AND NOT APPLE)
    file(RELATIVE_PATH _paraview_client_relpath
      "/prefix/${CMAKE_INSTALL_BINDIR}"
      "/prefix/${CMAKE_INSTALL_LIBDIR}")
    set(_paraview_client_origin_rpath
      "$ORIGIN/${_paraview_client_relpath}")

    list(APPEND CMAKE_INSTALL_RPATH
      "${_paraview_client_origin_rpath}")
  endif()

  add_executable("${name}")
  target_sources("${name}"
    PRIVATE
      ${ARGN})
  # Add a dummy file set to optimize dependencies. See CMP0154.
  _vtk_module_add_file_set("${name}"
    BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}"
    NAME      dummy)
  add_executable("LidarView::${name}" ALIAS "${name}")

  target_link_libraries("${name}"
    PRIVATE
      ParaView::RemotingApplication)

  target_link_libraries("${name}"
    PRIVATE
      ParaView::paraview_plugins
      LidarView::lidarview_plugins)

  target_compile_definitions("${name}"
    PRIVATE
    "PARAVIEW_USE_EXTERNAL_VTK=$<BOOL:${PARAVIEW_USE_EXTERNAL_VTK}>"
    "PARAVIEW_USE_PYTHON=$<BOOL:${PARAVIEW_USE_PYTHON}>")
  if (PARAVIEW_USE_PYTHON)
    target_link_libraries("${name}"
      PRIVATE
        VTK::PythonInterpreter
        ParaView::PythonInitializer
        LidarView::PythonInitializer
        ParaView::PythonInterpreterPath)
  endif ()

  install(
    TARGETS     "${name}"
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT   runtime
    EXPORT      LidarViewTools)
endfunction ()

foreach (exe IN ITEMS lvserver)
  lidarview_add_executable("${exe}" "${exe}.cxx")
endforeach ()

if (PARAVIEW_USE_PYTHON)
  foreach (exe IN ITEMS lvbatch lvpython)
    lidarview_add_executable("${exe}" "${exe}.cxx")
  endforeach ()
endif ()

