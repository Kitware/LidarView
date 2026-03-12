include(FileModuleInstaller)

python_module_install(
  NAME lidarviewexamples
  FILES
    __init__.py
    animation_attach_last_pose.py
    animation_focus_last_pose.py
    animation_non_temporal.py
    animation_static_temporal.py
    animation_temporal.py
    crop_and_export_example.py
    load_lidar_data.py
    plane_fit_and_process.py
    reset_window_views.py
    run_slam.py
    save_selected_point_on_multiple_frames.py
    split_window.py
  OUTPUT_DIRECTORY lidarviewexamples
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
