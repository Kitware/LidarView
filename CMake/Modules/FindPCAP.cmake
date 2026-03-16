# - Try to find libpcap include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(PCAP)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  PCAP_ROOT_DIR             Set this variable to the root installation of
#                            libpcap if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  PCAP_FOUND                System has libpcap, include and library dirs found
#  PCAP_INCLUDE_DIR          The libpcap include directories.
#  PCAP_LIBRARY              The libpcap library

find_path(PCAP_ROOT_DIR
  NAMES include/pcap.h
)

find_path(PCAP_INCLUDE_DIR
  NAMES pcap.h
  HINTS ${PCAP_ROOT_DIR}/include
)

set (HINT_DIR ${PCAP_ROOT_DIR}/lib)

# On x64 windows, we should look also for the .lib at /lib/x64/
# as this is the default path for the WinPcap developer's pack
if (${CMAKE_SIZEOF_VOID_P} EQUAL 8 AND WIN32)
  set (HINT_DIR ${PCAP_ROOT_DIR}/lib/x64/ ${HINT_DIR})
endif ()

find_library(PCAP_LIBRARY
  NAMES pcap wpcap
  HINTS ${HINT_DIR}
)

if (PCAP_LIBRARY AND PCAP_INCLUDE_DIR)
  add_library(PCAP::pcap UNKNOWN IMPORTED)
  set_target_properties(PCAP::pcap
    PROPERTIES
      IMPORTED_LOCATION "${PCAP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${PCAP_INCLUDE_DIR}")
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCAP DEFAULT_MSG
  PCAP_LIBRARY PCAP_INCLUDE_DIR
)

mark_as_advanced(
  PCAP_ROOT_DIR
  PCAP_INCLUDE_DIR
  PCAP_LIBRARY
)
