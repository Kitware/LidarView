# Only needed on OSX
# boost libs are missing when we try to launch lidarview app and tests. Even
# if all the needed cmake flags are used for lidarview to managed
# dependencies with RPATH, it remains an issue where RPATH are not used
# to find boost libs. This script has been created to force lidarview to
# search all boost dependencies along RPATH. (See related issue
# here https://gitlab.kitware.com/cmake/cmake/-/issues/19315).

# This script change paths for all boost dependencies from libboost_* to @rpath/libboost_* 
message(STATUS "Applying Reset rpath for all boost dependencies path")
get_filename_component(install_location "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)

# get a list of all boost lib used 
file(GLOB boost_libs LIST_DIRECTORIES false "${install_location}/lib/libboost*.dylib")
file(GLOB flann_libs LIST_DIRECTORIES false "${install_location}/lib/libflann*.dylib")

# get all app and tests on which to change boost path
file(GLOB app_list LIST_DIRECTORIES false "${install_location}/bin/*/Contents/MacOS/*")
file(GLOB lib_list LIST_DIRECTORIES false "lib/*.dylib" "${install_location}/lib/*.dylib")
file(GLOB test_list LIST_DIRECTORIES false "bin/Test*")

foreach (boost_lib IN LISTS boost_libs flann_libs)
  get_filename_component(libboost_name "${boost_lib}" NAME)
  MESSAGE("Setting RPATH for ${boost_lib}")
  foreach (bin_name IN LISTS app_list lib_list test_list)
    MESSAGE("  Set on bin ${bin_name}")
    execute_process(
      COMMAND install_name_tool
      -change "${libboost_name}" "@rpath/${libboost_name}"
      "${bin_name}")
  endforeach ()

endforeach ()
