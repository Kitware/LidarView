# Copy the scripts when installing
if(WIN32)
  set(install_destination "${LV_INSTALL_LIBRARY_DIR}/Lib/site-packages")
elseif(APPLE)
  set(install_destination "${LV_INSTALL_LIBRARY_DIR}/../Python")
else()
  set(install_destination "${LV_INSTALL_LIBRARY_DIR}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages")
endif()

set(scripts
    lib/camera_path.py
    lib/colormap_tools.py
    lib/temporal_animation_cue_helpers.py
    examples/example_non_temporal_animation.py
    examples/example_temporal_animation.py
    examples/example_static_temporal_animation.py
    examples/follow_last_pose.py
    examples/run_slam.py
)

install(FILES ${scripts} DESTINATION ${install_destination})
