include(FileModuleInstaller)

python_module_install(
  NAME lidarviewcore
  FILES
    lidarviewcore/__init__.py
    lidarviewcore/camera_path.py
    lidarviewcore/colormap_tools.py
    lidarviewcore/GetLandmarks.py
    lidarviewcore/kiwiviewerExporter.py
    lidarviewcore/matrix_rotation.py
    lidarviewcore/pipeline_setup_helpers.py
    lidarviewcore/temporal_animation_cue_helpers.py
    lidarviewcore/simple.py
    lidarviewcore/visualizationTools.py
  OUTPUT_DIRECTORY lidarviewcore
)

python_module_install(
  NAME lidarviewplugins
  FILES
    lidarviewplugins/__init__.py
    lidarviewplugins/SectionalView.py
  OUTPUT_DIRECTORY lidarviewplugins
)
