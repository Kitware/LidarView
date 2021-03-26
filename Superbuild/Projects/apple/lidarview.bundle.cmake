include(lidarview.bundle.common)
include(${LidarViewSuperBuild_SOURCE_DIR}/lidarview-superbuild/CMake/bundle/apple/LidarviewBundle.cmake)

# My understanding is that these module are not processed automatically
# by superbuild_apple_create_app because there is no path leading to
# them in binary LidarView or in any of its .dylib dependencies
set(lidarview_modules
    "LidarPluginPython.so"
    "libLidarPluginPythonD.dylib"
    "VelodynePluginPython.so"
    "libVelodynePluginPythonD.dylib")
    
foreach (module ${lidarview_modules})
  superbuild_apple_install_module(
    "\${CMAKE_INSTALL_PREFIX}"
    "${lidarview_appname}"
    "${superbuild_install_location}/bin/${lidarview_appname}/Contents/Libraries/${module}"
    "Contents/Libraries") # destination path inside bundle
endforeach()
