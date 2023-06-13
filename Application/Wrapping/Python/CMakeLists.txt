include(PythonModuleInstaller)

python_module_install(
  NAME lidarview
  FILES
    __init__.py
    aboutDialog.py
    applogic.py
    gridAdjustmentDialog.py
    planefit.py
    simple.py
  OUTPUT_DIRECTORY lidarview
)
