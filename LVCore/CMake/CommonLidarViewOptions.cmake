include(LidarViewSupportMacros)

#========================================================================
# BUILD OPTIONS:
# Options that affect the LidarView based app build, in general.
# These should begin with `LIDARVIEW_BUILD_`.
#========================================================================

lidarview_deprecated_setting(shared_default LIDARVIEW_BUILD_SHARED_LIBS BUILD_SHARED_LIBS "ON")
option(LIDARVIEW_BUILD_SHARED_LIBS "Build LidarView with shared libraries" "${shared_default}")

lidarview_deprecated_setting(doc_default LIDARVIEW_BUILD_DEVELOPER_DOCUMENTATION BUILD_DOC "OFF")
option(LIDARVIEW_BUILD_DEVELOPER_DOCUMENTATION "Generate LidarView C++/Python docs" "${doc_default}")

option(LIDARVIEW_BUILD_ALL_MODULES "Build all modules by default" ON)
mark_as_advanced(LIDARVIEW_BUILD_ALL_MODULES)

option(LIDARVIEW_BUILD_EXAMPLES "Build LidarView examples" OFF)

option(BUILD_TESTING "Enable testing" OFF)
mark_as_advanced(BUILD_TESTING)

lidarview_obsolete_setting(LIDARVIEW_BUILD_SLAM) # LidarView no longer build the Slam
option(PARAVIEW_PLUGIN_ENABLE_LidarSlam "Search for LidarSlam plugin." OFF)
mark_as_advanced(PARAVIEW_PLUGIN_ENABLE_LidarSlam)

#========================================================================
# OPTIONNAL EXTERN LIBRARY OPTIONS
#========================================================================
lidarview_deprecated_setting(pcl_default LIDARVIEW_USE_PCL ENABLE_pcl OFF)
option(LIDARVIEW_USE_PCL "PCL is required for some filters (e.g SLAM, PCLRansacFilter)" "${pcl_default}")

lidarview_deprecated_setting(ceres_default LIDARVIEW_USE_CERES ENABLE_ceres OFF)
option(LIDARVIEW_USE_CERES "Ceres is required for filters using non-linear least square optimization (e.g SLAM, autocalibration)" "${ceres_default}")

lidarview_deprecated_setting(nanoflann_default LIDARVIEW_USE_NANOFLANN ENABLE_nanoflann OFF)
option(LIDARVIEW_USE_NANOFLANN "Nanoflann will be required for filters using some optimized kdtree (e.g SLAM, DBSCAN)" "${nanoflann_default}")

lidarview_deprecated_setting(opencv_default LIDARVIEW_USE_OPENCV ENABLE_opencv OFF)
option(LIDARVIEW_USE_OPENCV "OpenCV is required for handling lidar-camera multisensor systems" "${opencv_default}")

option(LIDARVIEW_USE_PDAL "PDAL is required for writing .las files" OFF)

# The following libaries required for most LidarView functionnalities
option(LIDARVIEW_USE_PYTHONQT "PythonQt is required for QT python binding" ON)
mark_as_advanced(LIDARVIEW_USE_PYTHONQT)
option(LIDARVIEW_USE_PCAP "PCAP is required for reading .pcap (from lidar)" ON)
mark_as_advanced(LIDARVIEW_USE_PCAP)

lidarview_obsolete_setting(LIDARVIEW_USE_LIBLAS)

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
