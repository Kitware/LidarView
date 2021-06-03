# Bundling Scripts Stack Entry for $lidarview_appname - Unix Specific
include(lidarview.bundle.common)

# Trigger Unix-specific LidarView Bundling
include(${LidarViewSuperBuild_CMAKE_DIR}/bundle/unix/LidarviewBundle.cmake)

# LidarView-Unix Specifics

# Install Sensor calibration files
file(GLOB shared_files "${share_path}/*.xml")
install(FILES ${shared_files}
        DESTINATION "${share_dest}")
unset(shared_files)
