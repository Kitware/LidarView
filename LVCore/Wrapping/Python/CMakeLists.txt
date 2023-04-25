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
  NAME lidarviewpythonplugin
  FILES
    lidarviewpythonplugin/__init__.py
    lidarviewpythonplugin/filters/__init__.py
    lidarviewpythonplugin/filters/SectionalViewFilter.py
)
