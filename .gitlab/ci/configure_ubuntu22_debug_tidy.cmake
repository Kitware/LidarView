include("${CMAKE_CURRENT_LIST_DIR}/configure_ubuntu22_debug.cmake")

# Disable hesai plugin as external libary got a lot of warnings
set(PARAVIEW_ENABLE_PLUGIN_HesaiPlugin OFF CACHE BOOL "")

# Treats warnings as errors
# Note: -Wno-error=odr as one definition rule warning triggered by fpos_t only on ci
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Wno-error=odr" CACHE STRING "")
