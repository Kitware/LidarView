if (NOT (DEFINED lidarview_cmake_dir AND
         DEFINED lidarview_cmake_destination AND
         DEFINED lidarview_modules))
  message(FATAL_ERROR
    "LidarViewInstallCMakePackage is missing input variables.")
endif ()

set(lidarview_cmake_build_dir "${CMAKE_BINARY_DIR}/${lidarview_cmake_destination}")

_vtk_module_write_import_prefix("${lidarview_cmake_build_dir}/lidarview-prefix.cmake" "${lidarview_cmake_destination}")

string(REPLACE "LidarView::" "" _lidarview_all_components "${lidarview_modules}")

configure_file(
  "${lidarview_cmake_dir}/lidarview-config.cmake.in"
  "${lidarview_cmake_build_dir}/lidarview-config.cmake"
  @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${lidarview_cmake_build_dir}/lidarview-config-version.cmake"
  VERSION "${LIDARVIEW_VERSION_FULL}"
  COMPATIBILITY AnyNewerVersion)

configure_file(
  "${lidarview_cmake_dir}/FindLidarViewDependencies.cmake"
  "${lidarview_cmake_build_dir}/FindLidarViewDependencies.cmake"
  COPYONLY)
configure_file(
  "${lidarview_cmake_dir}/Modules/FindPCAP.cmake"
  "${lidarview_cmake_build_dir}/FindPCAP.cmake"
  COPYONLY)
configure_file(
  "${lidarview_cmake_dir}/Modules/FindTins.cmake"
  "${lidarview_cmake_build_dir}/FindTins.cmake"
  COPYONLY)

install(
  FILES       "${lidarview_cmake_build_dir}/lidarview-config.cmake"
              "${lidarview_cmake_build_dir}/lidarview-config-version.cmake"
              "${lidarview_cmake_build_dir}/lidarview-prefix.cmake"
              "${lidarview_cmake_build_dir}/FindLidarViewDependencies.cmake"
              "${lidarview_cmake_build_dir}/FindPCAP.cmake"
              "${lidarview_cmake_build_dir}/FindTins.cmake"
  DESTINATION "${lidarview_cmake_destination}"
  COMPONENT   "development")

vtk_module_export_find_packages(
  CMAKE_DESTINATION "${lidarview_cmake_destination}"
  FILE_NAME         "LidarView-vtk-module-find-packages.cmake"
  MODULES           ${lidarview_modules})
