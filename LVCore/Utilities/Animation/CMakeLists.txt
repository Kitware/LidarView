# Copy the scripts when installing

set(install_destination ${VV_INSTALL_LIBRARY_DIR}/site-packages)

set(scripts
        example_temporal_animation.py
        example_non_temporal_animation.py
        lib/camera_path.py
        lib/colormap_tools.py
        lib/temporal_animation_cue_helpers.py
)

install(FILES ${scripts} DESTINATION ${install_destination})
