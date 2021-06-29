#Sanitize checks
if(NOT LV_INSTALL_LIBRARY_DIR )
  message(FATAL_ERROR "LV_INSTALL_LIBRARY_DIR not set")
endif()
if(NOT LV_INSTALL_PYTHON_MODULES_DIR )
  message(FATAL_ERROR "LV_INSTALL_PYTHON_MODULES_DIR not set")
endif()

set(scripts
    lib/camera_path.py
    lib/colormap_tools.py
    lib/temporal_animation_cue_helpers.py
    examples/example_non_temporal_animation.py
    examples/example_temporal_animation.py
    examples/example_static_temporal_animation.py
    examples/attach_last_pose.py
    examples/focus_last_pose.py
    examples/run_slam.py
)

install(FILES ${scripts} DESTINATION ${LV_INSTALL_PYTHON_MODULES_DIR})
