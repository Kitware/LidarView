# Bundling Scripts Stack Entry for $lidarview_appname - Apple Specific
include(lidarview.bundle.common)

# Trigger Apple-specific LidarView Bundling
include(${LidarViewSuperBuild_CMAKE_DIR}/bundle/apple/LidarviewBundle.cmake)

# LidarView-Apple Specifics

# Install Sensor calibration files
file(GLOB shared_files "${share_path}/*.xml")
install(FILES ${shared_files}
        DESTINATION "${LV_INSTALL_RESOURCE_DIR}")
unset(shared_files)
