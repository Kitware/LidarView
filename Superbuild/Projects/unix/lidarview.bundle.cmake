include(lidarview.bundle.common)
include(${LidarViewSuperBuild_CMAKE_DIR}/bundle/unix/LidarviewBundle.cmake)

# Sensor calibration files
file(GLOB shared_files "${superbuild_install_location}/share/*.xml")
install(FILES ${shared_files}
        DESTINATION "share"
        COMPONENT superbuild)
unset(shared_files)
