include(lidarview.bundle.common)

# append non-common lidarview modules to be processed in LidarviewBundle
list(APPEND lidarview_modules "VelodynePluginPython.so")
list(APPEND lidarview_modules "libVelodynePluginPythonD.dylib")

include(${LidarViewSuperBuild_CMAKE_DIR}/bundle/apple/LidarviewBundle.cmake)

