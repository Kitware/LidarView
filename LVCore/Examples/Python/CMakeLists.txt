include(PythonModuleInstaller)

python_module_install(
  NAME lidarviewexamples
  FILES
    __init__.py
    crop_and_export_example.py
  OUTPUT_DIRECTORY lidarviewexamples
)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
