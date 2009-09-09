# This CMake script wants to use PC/SC functionality, therefore it looks 
# for PC/SC include files and libraries. 
#
# Operating Systems Supported:
# - Unix (requires pkg-config)
#   Tested with Ubuntu 9.04 and Fedora 11
# - Windows (requires MSVC)
#   Tested with Windows XP
#
# This should work for both 32 bit and 64 bit systems.
#
# Author: F. Kooman <fkooman@tuxed.net>
#

IF(MSVC)
  # Windows with Microsoft Visual C++
  FIND_PATH(PCSC_INCLUDE_DIRS WinSCard.h "$ENV{INCLUDE}")
  FIND_LIBRARY(PCSC_LIBRARIES NAMES WinSCard PATHS "$ENV{LIB}")
ELSE(MSVC)
  # If not MS Visual Studio we use PkgConfig
  FIND_PACKAGE (PkgConfig)
  IF(PKG_CONFIG_FOUND)
    PKG_CHECK_MODULES(PCSC REQUIRED libpcsclite)
  ELSE(PKG_CONFIG_FOUND)
    MESSAGE(FATAL_ERROR "Could not find PkgConfig")
  ENDIF(PKG_CONFIG_FOUND)
ENDIF(MSVC)

IF(PCSC_INCLUDE_DIRS AND PCSC_LIBRARIES)
   SET(PCSC_FOUND TRUE)
ENDIF(PCSC_INCLUDE_DIRS AND PCSC_LIBRARIES)

IF(PCSC_FOUND)
  IF(NOT PCSC_FIND_QUIETLY)
    MESSAGE(STATUS "Found PCSC: ${PCSC_LIBRARIES}")
  ENDIF(NOT PCSC_FIND_QUIETLY)
ELSE(PCSC_FOUND)
  IF(PCSC_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find PCSC")
  ENDIF(PCSC_FIND_REQUIRED)
ENDIF(PCSC_FOUND)
