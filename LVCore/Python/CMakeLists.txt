include(PythonModuleInstaller)

python_module_install(NAME lidarviewcore
                      FILES
                      lidarviewcore/__init__.py
                      lidarviewcore/DTMFilter/__init__.py
                      lidarviewcore/DTMFilter/DTMPythonFilter_Script.py
                      lidarviewcore/DTMFilter/DTM.py
                      lidarviewcore/GetLandmarks.py
                      lidarviewcore/kiwiviewerExporter.py
                      lidarviewcore/visualizationTools.py
                      )

python_module_install(NAME lidarviewpythonplugin
                      FILES
                      lidarviewpythonplugin/__init__.py
                      lidarviewpythonplugin/filters/__init__.py
                      lidarviewpythonplugin/filters/SectionalViewFilter.py
                      )
