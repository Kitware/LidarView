#========================================================================
# COMMON LIDARVIEW OPTIONS
#========================================================================
include(CommonLidarViewOptions)

#========================================================================
# OBSOLETE PLUGIN OPTIONS
#========================================================================
lidarview_deprecated_setting(velodyne_interpreter
  PARAVIEW_ENABLE_PLUGIN_VelodynePlugin LIDARVIEW_BUILD_VELODYNE "ON")
set(PARAVIEW_PLUGIN_ENABLE_VelodynePlugin "${velodyne_interpreter}" CACHE BOOL "")
lidarview_deprecated_setting(hesai_interpreter
  PARAVIEW_ENABLE_PLUGIN_HesaiPlugin LIDARVIEW_BUILD_HESAI "OFF")
set(PARAVIEW_PLUGIN_ENABLE_HesaiPlugin "${hesai_interpreter}" CACHE BOOL "")
