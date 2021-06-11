# Patch reset boost RPATH on OSX
# this is applied on LidarView app and tests
# for more information see comments in lidarview.osx-boost-rpath.cmake
install(SCRIPT "LVCore/CMake/scripts/lidarview.osx-boost-rpath.cmake")

# Patch ZLIB RPATH on Unix/OSX
# this is applied on LidarView app and tests
# for more information see comments in lidarview.unix-zlib-link.cmake
if(ENABLE_pcl AND UNIX AND NOT APPLE)
  message(WARNING "Forcing zlib.1.2.11 NEEDED using `patchelf` in Lidarplugin to avoid PCL/QT conflicts")
  install(SCRIPT "LVCore/CMake/scripts/lidarview.unix-zlib-link.cmake")
endif()

# Output CMakeCacheLog.txt
install(SCRIPT "LVCore/CMake/OutputCMakeCacheLog.cmake")
