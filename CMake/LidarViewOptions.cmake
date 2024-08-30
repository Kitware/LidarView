#========================================================================
# COMMON LIDARVIEW OPTIONS
#========================================================================
include(CommonLidarViewOptions)

#========================================================================
# LIDARVIEW BUILD OPTIONS
#========================================================================

lidarview_deprecated_setting(doc_default BUILD_DOCUMENTATION BUILD_DOC "OFF")
option(BUILD_DOCUMENTATION "Enable LidarView Documentation" ${doc_default})

#========================================================================
# OBSOLETE PLUGIN OPTIONS
#========================================================================
lidarview_obsolete_setting(LIDARVIEW_BUILD_VELODYNE)
lidarview_obsolete_setting(LIDARVIEW_BUILD_HESAI)
