option(ENABLE_slam "Build Lidar SLAM (requires PCL, Ceres, nanoflann; optional g2o)" OFF)
if(ENABLE_slam)

  set(SLAM_PARAVIEW_PLUGIN ON CACHE INTERNAL "")

  set(SLAM_INSTALL_INCLUDE_DIR "include" CACHE INTERNAL "")
  if (APPLE)
    set(SLAM_INSTALL_RUNTIME_DIR ${VV_INSTALL_RUNTIME_DIR}/${SOFTWARE_NAME}.app/Contents/Libraries/plugins CACHE INTERNAL "")
    set(SLAM_INSTALL_LIBRARY_DIR ${VV_INSTALL_LIBRARY_DIR}/${SOFTWARE_NAME}.app/Contents/Libraries/plugins CACHE INTERNAL "")
  else()
    set(SLAM_INSTALL_RUNTIME_DIR ${VV_INSTALL_RUNTIME_DIR}/plugins CACHE INTERNAL "")
    set(SLAM_INSTALL_LIBRARY_DIR ${VV_INSTALL_LIBRARY_DIR}/plugins CACHE INTERNAL "")
  endif()

  add_subdirectory(LidarSlam)

endif(ENABLE_slam)