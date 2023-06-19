include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")

set(lvsb_dir)
if (NOT DEFINED "ENV{LVSB_PATH}")
  get_filename_component(lvsb_dir "${CMAKE_CURRENT_LIST_DIR}/../../LVSB-DEPS/install/" ABSOLUTE)
else ()
  get_filename_component(lvsb_dir "$ENV{LVSB_PATH}/install/" ABSOLUTE)
endif ()
list(APPEND lvsb_dir "${lvsb_dir}/Python")
message(STATUS "CMAKE_PREFIX_PATH setted to ${lvsb_dir}")
set(CMAKE_PREFIX_PATH "${lvsb_dir}" CACHE PATH "")

set(Boost_USE_STATIC_LIBS OFF CACHE STRING "")

# Treats warnings as errors
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX /W4")
