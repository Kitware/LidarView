#--------------------------------------
# Scan and find LidarViewApp vtk modules
#--------------------------------------
set(lidarview_source_directories
  "${CMAKE_CURRENT_SOURCE_DIR}/Qt"
)

vtk_module_find_modules(lidarview_module_files ${lidarview_source_directories})
vtk_module_scan(
  MODULE_FILES          ${lidarview_module_files}
  PROVIDES_MODULES      lidarview_modules
  REQUIRES_MODULES      lidarview_required_modules
  UNRECOGNIZED_MODULES  lidarview_unrecognized_modules
  WANT_BY_DEFAULT       ON)

set(lidarview_unrecognized_lidarview_modules)
foreach (lidarview_module IN LISTS lidarview_unrecognized_modules)
  if (lidarview_module MATCHES "LidarViewApp::")
    list(APPEND lidarview_unrecognized_lidarview_modules
      "${lidarview_module}")
  endif ()
endforeach ()

if (lidarview_unrecognized_lidarview_modules)
  message(FATAL_ERROR
    "The following modules were requested or required, but not found: "
    "${lidarview_unrecognized_lidarview_modules}.")
endif ()

#--------------------------------------
# Build LidarViewApp vtk modules
#--------------------------------------
vtk_module_build(
  MODULES                   ${lidarview_modules}
  PACKAGE                   LidarViewApp
  INSTALL_EXPORT            LidarViewApp
  INSTALL_HEADERS           "${LIDARVIEW_INSTALL_DEVELOPMENT_FILES}"
  HEADERS_DESTINATION       "${CMAKE_INSTALL_INCLUDEDIR}/lidarview"
  CMAKE_DESTINATION         "${CMAKE_INSTALL_LIBDIR}/cmake/lidarview"
  VERSION                   "${LIDARVIEW_VERSION}"
  ENABLE_WRAPPING           OFF)

#--------------------------------------
# Build LidarView binary
#--------------------------------------
add_subdirectory(Client)

#--------------------------------------
# Export python wrapping
#--------------------------------------
add_subdirectory(Wrapping/Python)