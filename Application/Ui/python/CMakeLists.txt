include(PythonModuleInstaller)

python_module_install(
  NAME lidarview
  FILES
    lidarview/__init__.py
    lidarview/aboutDialog.py
    lidarview/applogic.py
    lidarview/gridAdjustmentDialog.py
    lidarview/planefit.py
)
