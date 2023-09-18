include("${CMAKE_CURRENT_LIST_DIR}/configure_ubuntu22_debug.cmake")

# Disable hesai plugin as external libary got a lot of warnings
set(PARAVIEW_ENABLE_PLUGIN_HesaiPlugin OFF CACHE BOOL "")

# Treats warnings as errors
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra" CACHE STRING "")
