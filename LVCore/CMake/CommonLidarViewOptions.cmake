include(LidarViewSupportMacros)

#========================================================================
# BUILD OPTIONS:
# Options that affect the LidarView based app build, in general.
# These should begin with `LIDARVIEW_BUILD_`.
#========================================================================

lidarview_deprecated_setting(shared_default BUILD_SHARED_LIBS LIDARVIEW_BUILD_SHARED_LIBS "ON")
option(BUILD_SHARED_LIBS "Build with shared libraries" "${shared_default}")

lidarview_deprecated_setting(doc_default LIDARVIEW_BUILD_DEVELOPER_DOCUMENTATION BUILD_DOC "OFF")
lidarview_deprecated_setting(doc_default BUILD_DEVELOPER_DOCUMENTATION LIDARVIEW_BUILD_DEVELOPER_DOCUMENTATION "OFF")
option(BUILD_DEVELOPER_DOCUMENTATION "Generate LidarView C++/Python docs" "${doc_default}")

lidarview_deprecated_setting(build_all_modules_default BUILD_ALL_MODULES LIDARVIEW_BUILD_ALL_MODULES "ON")
option(BUILD_ALL_MODULES "Build all modules by default" "${build_all_modules_default}")
mark_as_advanced(BUILD_ALL_MODULES)

lidarview_deprecated_setting(doc_default BUILD_EXAMPLES LIDARVIEW_BUILD_EXAMPLES "OFF")
option(BUILD_EXAMPLES "Build LidarView examples" "${doc_default}")

option(BUILD_TESTING "Enable testing" OFF)
mark_as_advanced(BUILD_TESTING)

#========================================================================
# OPTIONNAL EXTERN LIBRARY OPTIONS
#========================================================================
option(LIDARVIEW_USE_QT "If enabled build LidarView Qt interface" ON)

option(LIDARVIEW_USE_PYTHON "If enabled build LidarView python interface" ON)

lidarview_deprecated_setting(ceres_default LIDARVIEW_USE_CERES ENABLE_ceres OFF)
option(LIDARVIEW_USE_CERES "Ceres is required for filters using non-linear least square optimization (e.g SLAM, autocalibration)" "${ceres_default}")

lidarview_deprecated_setting(nanoflann_default LIDARVIEW_USE_NANOFLANN ENABLE_nanoflann OFF)
option(LIDARVIEW_USE_NANOFLANN "Nanoflann will be required for filters using some optimized kdtree (e.g SLAM, DBSCAN)" "${nanoflann_default}")

lidarview_deprecated_setting(opencv_default LIDARVIEW_USE_OPENCV ENABLE_opencv OFF)
option(LIDARVIEW_USE_OPENCV "OpenCV is required for handling lidar-camera multisensor systems" "${opencv_default}")

option(LIDARVIEW_USE_PDAL "PDAL is required for writing .las files" OFF)

option(LIDARVIEW_USE_YAMLCPP "Yaml-cpp is required for reading config yaml files (used in camera calibration)" OFF)

lidarview_obsolete_setting(LIDARVIEW_BUILD_SLAM) # LidarView no longer build the Slam
lidarview_deprecated_setting(slam_default LIDARVIEW_USE_LIDARSLAM_PLUGIN PARAVIEW_PLUGIN_ENABLE_LidarSlam "OFF")
option(LIDARVIEW_USE_LIDARSLAM_PLUGIN "Search for LidarSlam plugin." "${slam_default}")

if (WIN32)
  option(LIDARVIEW_USE_NPCAP "Use npcap instead of winpcap." OFF)
endif ()

lidarview_obsolete_setting(LIDARVIEW_USE_PCAP)
lidarview_obsolete_setting(LIDARVIEW_USE_LIBLAS)
lidarview_obsolete_setting(LIDARVIEW_USE_PYTHONQT)

#========================================================================
# LidarCorePlugin option required for LidarView filters
#========================================================================
set(PARAVIEW_PLUGIN_ENABLE_LidarCorePlugin ON CACHE BOOL "")
mark_as_advanced(PARAVIEW_PLUGIN_ENABLE_LidarCorePlugin)

#========================================================================
# MISCELLANEOUS OPTIONS:
# Options that are hard to classify. Keep this list minimal.
# These should be advanced by default.
#========================================================================
option(LIDARVIEW_INSTALL_DEVELOPMENT_FILES "Install development files to the install tree" ON)
mark_as_advanced(LIDARVIEW_INSTALL_DEVELOPMENT_FILES)

option(LIDARVIEW_HIDE_DEPRECATION_WARNINGS "If ON hide all deprecation warnings" OFF)
mark_as_advanced(LIDARVIEW_HIDE_DEPRECATION_WARNINGS)
