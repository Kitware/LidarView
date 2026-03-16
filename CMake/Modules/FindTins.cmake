# - Try to find libtins include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(Tins)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
# Variables defined by this module:
#
#  Tins_FOUND                System has libtins, include and library dirs found
#  Tins_INCLUDE_DIR          The libtins include directories.
#  Tins_LIBRARY              The libtins library
#  Tins_VERSION              The libtins version

find_package(libtins REQUIRED)

if (TARGET tins)
  set(Tins_LIBRARY tins)
  set(Tins_INCLUDE_DIR "${LIBTINS_INCLUDE_DIRS}")
  set(Tins_VERSION "${libtins_VERSION}")

  set(include_directories "${LIBTINS_INCLUDE_DIRS}")
  if (NOT TARGET PCAP::pcap)
    find_package(PCAP REQUIRED)
    list(APPEND include_directories "${PCAP_INCLUDE_DIR}")
  endif ()

  set_target_properties(tins
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${include_directories}")
  add_library(Tins::tins ALIAS tins)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Tins DEFAULT_MSG
  Tins_LIBRARY Tins_INCLUDE_DIR Tins_VERSION
)

mark_as_advanced(
  Tins_LIBRARY
  Tins_INCLUDE_DIR
)
