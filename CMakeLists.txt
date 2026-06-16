cmake_minimum_required(VERSION 3.20.3 FATAL_ERROR)
project(LidarView)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(LidarViewOptions)
include(FindLidarViewDependencies)

#--------------------------------------
# Set-up test & cmake variables
#--------------------------------------
include(LidarViewSetBuildVariable)
set(lidarview_plugins_library_subdirectory "lidarview/plugins")
set(lidarview_cmake_dir "${CMAKE_SOURCE_DIR}/CMake")
set(lidarview_cmake_destination "${CMAKE_INSTALL_LIBDIR}/cmake/lidarview")
set(lidarview_test_data_directory_input "${CMAKE_BINARY_DIR}/Testing/Data")
set(lidarview_test_data_directory_output "${lidarview_test_data_directory_input}")
set(lidarview_test_output_directory "${CMAKE_BINARY_DIR}/Testing/Temporary")

if (BUILD_TESTING)
  include(CTest)
  include(FetchTestData)
  lidarview_fetch_test_data(
    DATA_NAME "lidarview"
    COMMIT_SHA "25d7882a6527aa52bd4b1b933e1afd982dcc7268"
  )
  set(lidarview_test_data_directory_input "${FETCH_TEST_DATA_DIR}")
  set(lidarview_test_data_directory_output "${lidarview_test_data_directory_input}")
  set(lidarview_test_baseline_dir "${lidarview_test_data_directory_input}/Baseline")
endif ()

#--------------------------------------
# Scan LidarView plugins
#--------------------------------------
set(lidarview_default_plugins
  LidarCorePlugin
  DatasetIOPlugin
  VelodynePlugin
  LeishenPlugin
  LivoxPlugin
  RobosensePlugin
  HesaiPlugin
)

if (LIDARVIEW_USE_CURL)
  list(APPEND lidarview_delayed_plugins
    OusterPlugin
)
endif()

# Ignore following default plugins in autoload default.
set(lidarview_default_autoload_plugins ${lidarview_default_plugins})

list(APPEND lidarview_default_plugins
  MotionDetectorToolbox
)

if (BUILD_EXAMPLES)
  list(APPEND lidarview_default_plugins
    AverageSelectedPointsPlugin
    ProcessingSamplePlugin
    RotativeLidarInterpreter
    ThresholdFromFilePlugin
    TimeBasedLidarInterpreter)

  if (PARAVIEW_USE_QT)
    list(APPEND lidarview_default_plugins
      CustomDockWidget)
  endif ()
endif ()

# Ensure that the LidarCorePlugin is positioned at the first position,
# interpreter plugins depend on LidarCorePlugin having its XML loaded.
set(lidarview_plugin_directories
  "${CMAKE_CURRENT_SOURCE_DIR}/LidarCore/Plugin/"
  "${CMAKE_CURRENT_SOURCE_DIR}/Plugins/"
)

if (BUILD_EXAMPLES)
  list(APPEND lidarview_plugin_directories
    "${CMAKE_CURRENT_SOURCE_DIR}/Examples/Plugins/")
endif ()

foreach (lidarview_default_plugin IN LISTS lidarview_default_plugins)
  set("_paraview_plugin_default_${lidarview_default_plugin}" "ON")
endforeach ()

paraview_plugin_find_plugins(lidarview_plugin_files ${lidarview_plugin_directories})
paraview_plugin_scan(
  PLUGIN_FILES
    ${lidarview_plugin_files}
  PROVIDES_PLUGINS  lidarview_plugins
  REQUIRES_MODULES  lidarview_plugin_required_modules)

#--------------------------------------
# Scan and find vtk modules
#--------------------------------------
list(APPEND lidarview_source_directories
  "${CMAKE_CURRENT_SOURCE_DIR}/LidarCore"
  "${CMAKE_CURRENT_SOURCE_DIR}/Qt"
)

vtk_module_find_modules(lidarplugin_module_files ${lidarview_source_directories})
vtk_module_scan(
  MODULE_FILES          ${lidarplugin_module_files}
  REQUEST_MODULES       ${lidarview_plugin_required_modules}
                        ${lidarview_client_required_modules}
  PROVIDES_MODULES      lidarview_modules
  REQUIRES_MODULES      lidarview_required_modules
  UNRECOGNIZED_MODULES  lidarview_unrecognized_modules
  WANT_BY_DEFAULT       "${BUILD_ALL_MODULES}"
  ENABLE_TESTS          "${BUILD_TESTING}")

set(lidarview_unrecognized_lidarview_modules)
foreach (lidarview_module IN LISTS lidarview_unrecognized_modules)
  if (lidarview_module MATCHES "LidarView::")
    list(APPEND lidarview_unrecognized_lidarview_modules
      "${lidarview_module}")
  endif ()
endforeach ()

if (lidarview_unrecognized_lidarview_modules)
  list(REMOVE_ITEM lidarview_unrecognized_lidarview_modules ${lidarview_rejected_modules})
endif ()

if (lidarview_unrecognized_lidarview_modules)
  message(FATAL_ERROR
    "The following modules were requested or required, but not found: "
    "${lidarview_unrecognized_lidarview_modules}.")
endif ()

#--------------------------------------
# Build LidarView vtk modules
#--------------------------------------
vtk_module_build(
  MODULES                   ${lidarview_modules}
  INSTALL_EXPORT            LidarView
  INSTALL_HEADERS           "${LIDARVIEW_INSTALL_DEVELOPMENT_FILES}"
  HEADERS_DESTINATION       "${CMAKE_INSTALL_INCLUDEDIR}/lidarview"
  CMAKE_DESTINATION         "${lidarview_cmake_destination}"
  LICENSE_DESTINATION       "${CMAKE_INSTALL_LICENSEDIR}"
  VERSION                   "${LIDARVIEW_VERSION}"
  SOVERSION                 "1"
  ENABLE_WRAPPING           ON
  TEST_INPUT_DATA_DIRECTORY   "${lidarview_test_data_directory_input}"
  TEST_OUTPUT_DATA_DIRECTORY  "${lidarview_test_data_directory_output}"
  TEST_OUTPUT_DIRECTORY       "${lidarview_test_output_directory}")

#--------------------------------------
# Build LidarView plugins
#--------------------------------------
foreach (module IN LISTS lidarview_plugin_required_modules)
  if (NOT TARGET "${module}")
    message(FATAL_ERROR "Missing required module: ${module}")
  endif ()
endforeach ()

set(autoload_plugins)
foreach (lidarview_plugin IN LISTS lidarview_plugins)
  set(_autoload_plugin OFF)
  if (lidarview_plugin IN_LIST lidarview_default_autoload_plugins)
    set(_autoload_plugin ON)
  endif ()

  option("PARAVIEW_PLUGIN_AUTOLOAD_${lidarview_plugin}" "Autoload the ${lidarview_plugin} plugin" ${_autoload_plugin})
  mark_as_advanced("PARAVIEW_PLUGIN_AUTOLOAD_${lidarview_plugin}")

  if (PARAVIEW_PLUGIN_AUTOLOAD_${lidarview_plugin})
    list(APPEND autoload_plugins
      "${lidarview_plugin}")
  endif ()
endforeach ()

paraview_plugin_build(
  HEADERS_DESTINATION   "${CMAKE_INSTALL_INCLUDEDIR}/lidarview"
  RUNTIME_DESTINATION   "${CMAKE_INSTALL_BINDIR}"
  LIBRARY_DESTINATION   "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY_SUBDIRECTORY  "${lidarview_plugins_library_subdirectory}"
  CMAKE_DESTINATION     "${lidarview_cmake_destination}"
  PLUGINS_FILE_NAME     "lidarview.plugins.xml"
  TARGET                LidarView::lidarview_plugins
  INSTALL_EXPORT        LidarViewPlugins
  INSTALL_HEADERS       "${LIDARVIEW_INSTALL_DEVELOPMENT_FILES}"
  TARGET_COMPONENT      "development"
  PLUGINS_COMPONENT     "plugins"
  PLUGINS               ${lidarview_plugins}
  AUTOLOAD              ${autoload_plugins}
)

if (LIDARVIEW_USE_PYTHON)
  #-----------------------------------------------------------------------------
  # Python wrapping
  # Simply use "import lidarview.modules" and you have acces to the wrapped c++ code
  # DO NOT FOR ANY REASON USE the method GetClientSideObject() to have acces to the vtkObject directly.
  #-----------------------------------------------------------------------------

  vtk_module_wrap_python(
    MODULES             ${lidarview_modules}
    INSTALL_EXPORT      LidarViewPython
    INSTALL_HEADERS     "${LIDARVIEW_INSTALL_DEVELOPMENT_FILES}"
    CMAKE_DESTINATION   "${lidarview_cmake_destination}"
    LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lidarview/lvpythonmodules"
    PYTHON_PACKAGE      "lidarview.modules"
    MODULE_DESTINATION  "${LIDARVIEW_PYTHON_SITE_PACKAGES_SUFFIX}"
    TARGET              LidarView::lvpythonmodules
    DEPENDS             VTK::vtkpythonmodules
  )

  # Add sudirectory containing python helper functions for pipeline scripting
  add_subdirectory(Wrapping/Python)

  if (BUILD_EXAMPLES)
    add_subdirectory(Examples/Python)
  endif ()
endif ()

#--------------------------------------
# Install development files if needeed
#--------------------------------------
if (LIDARVIEW_INSTALL_DEVELOPMENT_FILES)
  include(LidarViewInstallCMakePackage)
endif ()

#--------------------------------------
# Build Python & UI XML Testing
#--------------------------------------
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()

# Build lidarview application & QT module specific
add_subdirectory(Application)

if (BUILD_DOCUMENTATION)
  add_subdirectory(Utilities/Sphinx)
endif ()

if (BUILD_DEVELOPER_DOCUMENTATION)
  add_subdirectory(Utilities/Doxygen)
endif ()
