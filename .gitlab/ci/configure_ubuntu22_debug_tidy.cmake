include("${CMAKE_CURRENT_LIST_DIR}/configure_ubuntu22_debug.cmake")

# Treats warnings as errors
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra" CACHE STRING "")

# Create a compilation database for the analize stage (clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "")
