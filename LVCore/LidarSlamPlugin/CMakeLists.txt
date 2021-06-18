#Sanitize checks
if(NOT LV_INSTALL_LIBRARY_DIR)
  message(FATAL_ERROR "Var LV_INSTALL_LIBRARY_DIR not set")
endif()

option(LIDARVIEW_BUILD_SLAM "Build Lidar SLAM (requires PCL, Ceres, nanoflann; optional g2o)" OFF)
if(LIDARVIEW_BUILD_SLAM)

  # Notify to build Paraview LidarSlamPlugin
  set(SLAM_PARAVIEW_PLUGIN ON CACHE INTERNAL "")
  # Set install directories
  set(SLAM_INSTALL_LIBRARY_DIR ${LV_INSTALL_LIBRARY_DIR} CACHE INTERNAL "")
  if (APPLE)
    set(SLAM_INSTALL_PARAVIEW_PLUGIN_DIR ${LV_INSTALL_LIBRARY_DIR}/../Plugins CACHE INTERNAL "")
  else()
    set(SLAM_INSTALL_PARAVIEW_PLUGIN_DIR ${LV_INSTALL_LIBRARY_DIR}/plugins CACHE INTERNAL "")
  endif()
  # Build LidarSlam lib as STATIC in order to have a standalone LidarSlamPlugin
  set(BUILD_SHARED_LIBS OFF)

  # LidarSlam could use g2o in the future relying on ENABLE_g2o, We disable this for now and keep this line for future reference.
  set(ENABLE_g2o OFF)

  # Build SLAM lib and PV plugin
  add_subdirectory(LidarSlam)

endif()
