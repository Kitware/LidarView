# Bundling Scripts Stack Entry for $lidarview_appname - Apple Specific
include(lidarview.bundle.common)

# append non-common lidarview modules to be processed in LidarviewBundle
list(APPEND lidarview_modules "VelodynePluginPython.so")
list(APPEND lidarview_modules "libVelodynePluginPythonD.dylib")

# Trigger Apple-specific LidarView Bundling
include(${LidarViewSuperBuild_CMAKE_DIR}/bundle/apple/LidarviewBundle.cmake)

