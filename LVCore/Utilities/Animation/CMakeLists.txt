# Copy the scripts when installing

set(install_destination ${VV_INSTALL_LIBRARY_DIR}/site-packages)

set(scripts
    lib/camera_path.py
    lib/colormap_tools.py
    lib/temporal_animation_cue_helpers.py
    examples/example_non_temporal_animation.py
    examples/example_temporal_animation.py
    examples/example_static_temporal_animation.py
)

install(FILES ${scripts} DESTINATION ${install_destination})
