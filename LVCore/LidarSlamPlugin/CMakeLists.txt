option(ENABLE_new_slam "Build new Lidar Slam (requires PCL, Ceres, nanoflann, g2o, ...)" OFF)
if(ENABLE_new_slam)

  set(SLAM_PARAVIEW_PLUGIN ON CACHE INTERNAL "")

  if (APPLE)
    set(SLAM_INSTALL_RUNTIME_DIR ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Libraries CACHE INTERNAL "")
    set(SLAM_INSTALL_LIBRARY_DIR ${VV_INSTALL_LIBRARY_DIR}/${SOFTWARE_NAME}.app/Contents/Libraries CACHE INTERNAL "")
  else()
    set(SLAM_INSTALL_RUNTIME_DIR ${VV_INSTALL_RUNTIME_DIR} CACHE INTERNAL "")
    set(SLAM_INSTALL_LIBRARY_DIR ${VV_INSTALL_LIBRARY_DIR} CACHE INTERNAL "")
  endif()

  add_subdirectory(LidarSlam)

endif(ENABLE_new_slam)