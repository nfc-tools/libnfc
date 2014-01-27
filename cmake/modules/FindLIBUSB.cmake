# This CMake script wants to use libusb functionality, therefore it looks 
# for libusb include files and libraries. 
#
# Operating Systems Supported:
# - Unix (requires pkg-config)
#   Tested with Ubuntu 9.04 and Fedora 11
# - Windows (requires MinGW)
#   Tested with Windows XP/Windows 7
#
# This should work for both 32 bit and 64 bit systems.
#
# Author: F. Kooman <fkooman@tuxed.net>
#

# FreeBSD has built-in libusb since 800069
IF(CMAKE_SYSTEM_NAME MATCHES FreeBSD)
  EXEC_PROGRAM(sysctl ARGS -n kern.osreldate OUTPUT_VARIABLE FREEBSD_VERSION)
  SET(MIN_FREEBSD_VERSION 800068)
  IF(FREEBSD_VERSION GREATER ${MIN_FREEBSD_VERSION})
    SET(LIBUSB_INCLUDE_DIRS "/usr/include")
    SET(LIBUSB_LIBRARIES "usb")
    SET(LIBUSB_LIBRARY_DIRS "/usr/lib/")
  ENDIF(FREEBSD_VERSION GREATER ${MIN_FREEBSD_VERSION})
ENDIF(CMAKE_SYSTEM_NAME MATCHES FreeBSD)

IF(NOT LIBUSB_FOUND)
  FIND_PACKAGE (PkgConfig)
  
  IF(PKG_CONFIG_FOUND)
    PKG_CHECK_MODULES(LIBUSB REQUIRED libusb-1.0)
  ENDIF()

  FIND_PATH(LIBUSB_INCLUDE_DIRS libusb.h lusb0_usb.h
    PATHS 
      ${PC_LIBUSB_INCLUDEDIR} ${PC_LIBUSB_INCLUDE_DIRS}
      "$ENV{ProgramFiles}/LibUSB-Win32/include"
    PATH_SUFFIXES libusb-1.0
  )
  
  FIND_LIBRARY(LIBUSB_LIBRARIES NAMES usb-1.0 libusb
    PATHS ${PC_LIBUSB_LIBDIR} ${PC_LIBUSB_LIBRARY_DIRS}
    "$ENV{ProgramFiles}/LibUSB-Win32/lib/gcc"
  )
  
ENDIF()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBUSB DEFAULT_MSG LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIRS)

MARK_AS_ADVANCED(LIBUSB_INCLUDE_DIRS LIBUSB_LIBRARIES)
