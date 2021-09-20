# Bundling Scripts Stack Entry for $lidarview_appname - Win32 Specific
include(lidarview.bundle.common)

# Trigger Win32-specific LidarView Bundling
include(${LidarViewSuperBuild_CMAKE_DIR}/bundle/win32/LidarviewBundle.cmake)

# LidarView-Win32 Specifics

# Install Sensor calibration files
file(GLOB shared_files "${share_path}/*.xml")
install(FILES ${shared_files}
        DESTINATION "${LV_INSTALL_RESOURCE_DIR}")
unset(shared_files)
