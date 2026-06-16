include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")

# Treats warnings as errors
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra" CACHE STRING "")

# Ignore deprecation warning to be able to build
set(LIDARVIEW_HIDE_DEPRECATION_WARNINGS ON CACHE BOOL "")

# Create a compilation database for the analize stage (clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "")
