
set(lidarview_python_files
  lidarview/__init__.py
  lidarview/applogic.py
  lidarview/gridAdjustmentDialog.py
  lidarview/kiwiviewerExporter.py
  lidarview/planefit.py
  lidarview/timer.py
  lidarview/aboutDialog.py
  lidarview/DTMFilter/__init__.py
  lidarview/DTMFilter/DTMPythonFilter_Script.py
  lidarview/DTMFilter/DTM.py
  lidarview/visualizationTools.py
  )

set(python_module_dir "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages")

# Copy python files
set(copied_python_files)
foreach(file ${lidarview_python_files})
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
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/compile_all.py.in"
               ${compile_all_script}
               @ONLY IMMEDIATE)

add_custom_command(
  COMMAND ${PYTHON_EXECUTABLE}
  ARGS  "${compile_all_script}"
  DEPENDS ${copied_python_files}  ${compile_all_script}
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/compile_complete"
  )

add_custom_target(python_byte_compile ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/compile_complete")


# Install python module
set(install_destination ${VV_INSTALL_LIBRARY_DIR}/site-packages)
if (APPLE)
  set(install_destination ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Python)
endif()

install(DIRECTORY  ${python_module_dir}/lidarview
        DESTINATION ${install_destination}
        COMPONENT Runtime
        USE_SOURCE_PERMISSIONS)
