if (NOT DEFINED "${LVSB_PATH}")
  message(FATAL_ERROR "LVSB_PATH is not defined.")
else()
  get_filename_component(lvsb_deps_path "${LVSB_PATH}" ABSOLUTE)
endif()

# Remove current deps folder
message(STATUS "Cleaning old dependencies folder")
file(REMOVE_RECURSE ${lvsb_deps_path})

set(lvsb_deps_binary_dir "${lvsb_deps_path}")
set(lvsb_deps_source_dir "${lvsb_deps_path}/src")
set(lvsb_deps_subbuild_dir "${lvsb_deps_path}/subbuild")

message(STATUS "Cloning lvsb repository to build LidarView dependencies")
include(FetchContent)
FetchContent_Populate(
  lvsb_deps
  GIT_REPOSITORY https://gitlab.kitware.com/LidarView/lidarview-superbuild.git
  GIT_TAG master
  BINARY_DIR ${lvsb_deps_binary_dir}
  SOURCE_DIR ${lvsb_deps_source_dir}
  SUBBUILD_DIR ${lvsb_deps_subbuild_dir}
  QUIET
)
