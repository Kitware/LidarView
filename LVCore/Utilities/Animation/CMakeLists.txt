set(scripts
  examples/example_non_temporal_animation.py
  examples/example_temporal_animation.py
  examples/example_static_temporal_animation.py
  examples/attach_last_pose.py
  examples/focus_last_pose.py
  examples/run_slam.py
)

install(FILES ${scripts} DESTINATION ${LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX})
