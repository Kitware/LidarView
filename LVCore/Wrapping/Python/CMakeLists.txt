include(PythonModuleInstaller)

python_module_install(
  NAME lidarviewcore
  FILES
    lidarviewcore/__init__.py
    lidarviewcore/GetLandmarks.py
    lidarviewcore/kiwiviewerExporter.py
    lidarviewcore/simple.py
    lidarviewcore/visualizationTools.py
)

python_module_install(
  NAME lidarviewplugins
  FILES
    lidarviewplugins/__init__.py
    lidarviewplugins/SectionalView.py
)
