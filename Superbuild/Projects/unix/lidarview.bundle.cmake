# Bundling Scripts Stack Entry for $lidarview_appname - Unix Specific
include(lidarview.bundle.common)

# Trigger Unix-specific LidarView Bundling
include(${LidarViewSuperBuild_CMAKE_DIR}/bundle/unix/LidarviewBundle.cmake)

# LidarView-Unix Specifics

# Sensor calibration files
file(GLOB shared_files "${superbuild_install_location}/share/*.csv")
install(FILES ${shared_files}
        DESTINATION "${LV_INSTALL_RESOURCE_DIR}")
unset(shared_files)
file(GLOB shared_files "${superbuild_install_location}/share/*.xml")
install(FILES ${shared_files}
        DESTINATION "${LV_INSTALL_RESOURCE_DIR}")
unset(shared_files)
