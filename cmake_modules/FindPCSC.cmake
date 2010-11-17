# - Try to find the PC/SC smart card  library
# Once done this will define
#
#  PCSC_FOUND - system has the PC/SC library
#  PCSC_INCLUDE_DIRS - the PC/SC include directory
#  PCSC_LIBRARIES - The libraries needed to use PC/SC
#
# Author: F. Kooman <fkooman@tuxed.net>
# Version: 20101019
#

FIND_PACKAGE (PkgConfig)
IF(PKG_CONFIG_FOUND)
    # Will find PC/SC library on Linux/BSDs using PkgConfig
    PKG_CHECK_MODULES(PCSC libpcsclite)
#   PKG_CHECK_MODULES(PCSC QUIET libpcsclite)   # IF CMake >= 2.8.2?
ENDIF(PKG_CONFIG_FOUND)

IF(NOT PCSC_FOUND)
   # Will find PC/SC headers both on Mac and Windows
   FIND_PATH(PCSC_INCLUDE_DIRS WinSCard.h)
   # PCSC library is for Mac, WinSCard library is for Windows
   FIND_LIBRARY(PCSC_LIBRARIES NAMES PCSC WinSCard)
   
   IF(MINGW)
     # MinGW32 with PCSC framework use Microsoft SDK's WinSCard.h
     SET(PCSC_INCLUDE_DIRS)
     SET(PCSC_INCLUDE_DIRS "$ENV{ProgramFiles}/Microsoft SDKs/Windows/v7.0/Include")
   ENDIF(MINGW)
ENDIF(NOT PCSC_FOUND)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCSC DEFAULT_MSG
  PCSC_LIBRARIES
  PCSC_INCLUDE_DIRS
)
MARK_AS_ADVANCED(PCSC_INCLUDE_DIRS PCSC_LIBRARIES)
