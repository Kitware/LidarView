option(ENABLE_slam "Build Lidar SLAM (requires PCL, Ceres, nanoflann; optional g2o)" OFF)
if(ENABLE_slam)

  set(SLAM_PARAVIEW_PLUGIN ON CACHE INTERNAL "")
  set(SLAM_INSTALL_LIBRARY_DIR ${LV_INSTALL_LIBRARY_DIR} CACHE INTERNAL "")

  if (APPLE)
    set(SLAM_INSTALL_PARAVIEW_PLUGIN_DIR ${LV_INSTALL_LIBRARY_DIR}/../Plugins CACHE INTERNAL "")
  else()
    set(SLAM_INSTALL_PARAVIEW_PLUGIN_DIR ${LV_INSTALL_LIBRARY_DIR}/plugins CACHE INTERNAL "")
  endif()

  add_subdirectory(LidarSlam)

endif(ENABLE_slam)