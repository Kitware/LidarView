include(PythonModuleInstaller)

python_module_install(
  NAME lidarview
  FILES
    __init__.py
    applogic.py
    planefit.py
    simple.py
  OUTPUT_DIRECTORY lidarview
)
