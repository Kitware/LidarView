# Only needed on UNIX (not APPLE)
# Fix a conflict between the built libz V1.2.11, and the one used by a SYSTEM Qt5,
# Only occurs on a system with zlib inferior to 1.2.9 (E.g Ubuntu 16.04).
# This only applies at runtime on the machine in question (Tests), not for packages
# 
# The Fix links makes libLidarPlugin.so rely explicitely on the built libz.so.1.2.11
# Requires `sudo apt-get install patchelf` 

# Sanitize
if(NOT UNIX)
  message(FATAL_ERROR "This patch only Applies to UNIX")
endif()
find_program(HAS_PATCHELF "patchelf")
IF(NOT HAS_PATCHELF)
  message(FATAL_ERROR "patchelf utility not installed")
endif()

message(STATUS "Applying Zlib link fix to every test binaries path")
get_filename_component(install_location "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)

# get all app and tests on which to change paths
file(GLOB app_list LIST_DIRECTORIES false "${install_location}/bin/*")
file(GLOB test_list LIST_DIRECTORIES false "bin/Test*")

foreach (binary_file IN LISTS app_list test_list)
  get_filename_component(libboost_name "${binary_file}" NAME)
  MESSAGE("Setting link for ${binary_file}")
  
  execute_process(
    COMMAND patchelf --add-needed "libz.so.1.2.11" "${binary_file}"
  )
endforeach ()
