# reset boost RPATH on OSX
# this is applied on LidarView app and tests
# for more information see comments in lidarview.osx-boost-rpath.cmake
install(SCRIPT "LVCore/CMake/scripts/lidarview.osx-boost-rpath.cmake")

# Output CMakeCacheLog.txt
install(SCRIPT "LVCore/CMake/OutputCMakeCacheLog.cmake")
