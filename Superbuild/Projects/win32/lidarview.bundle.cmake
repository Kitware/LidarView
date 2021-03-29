include(lidarview.bundle.common)
include(${LidarViewSuperBuild_CMAKE_DIR}/bundle/win32/LidarviewBundle.cmake)

# Sensor calibration files
file(GLOB shared_files "${superbuild_install_location}/share/*.xml")
install(FILES ${shared_files}
        DESTINATION "share"
        COMPONENT superbuild
        )
unset(shared_files)

# some dlls are missing.
# They should be installed automatically because of paraview/vtk's cmake lists
file(GLOB vtk_dlls "${superbuild_install_location}/bin/vtk*.dll")
install(FILES ${vtk_dlls}
        DESTINATION "bin"
        COMPONENT superbuild
        )
        
set(lidarview_modules
    "${superbuild_install_location}/bin/LidarPluginPython.pyd"
    "${superbuild_install_location}/bin/LidarPluginPythonD.dll"
    "${superbuild_install_location}/bin/VelodynePlugin.dll"
    "${superbuild_install_location}/bin/VelodynePluginPython.pyd"
    "${superbuild_install_location}/bin/VelodynePluginPythonD.dll"
    )
install(FILES ${lidarview_modules}
        DESTINATION "bin"
        COMPONENT superbuild
        )

