include(PythonModuleInstaller)

python_module_install(
  NAME lidarviewcore
  PATH ${CMAKE_CURRENT_LIST_DIR}/lidarviewcore)

python_module_install(
  NAME lidarviewpythonplugin
  PATH ${CMAKE_CURRENT_LIST_DIR}/lidarviewpythonplugin)
