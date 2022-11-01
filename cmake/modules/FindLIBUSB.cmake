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
    SET(LIBUSB_FOUND TRUE)
    SET(LIBUSB_INCLUDE_DIRS "/usr/include")
    SET(LIBUSB_LIBRARIES "usb")
    SET(LIBUSB_LIBRARY_DIRS "/usr/lib/")
  ENDIF(FREEBSD_VERSION GREATER ${MIN_FREEBSD_VERSION})
ENDIF(CMAKE_SYSTEM_NAME MATCHES FreeBSD)

IF(NOT LIBUSB_FOUND)
  IF(WIN32)
    IF(MINGW)
      FIND_PATH(LIBUSB_INCLUDE_DIRS lusb0_usb.h "${CMAKE_CURRENT_BINARY_DIR}/LibUSB-Win32/include" NO_SYSTEM_ENVIRONMENT_PATH)
      FIND_LIBRARY(LIBUSB_LIBRARIES NAMES libusb PATHS "${CMAKE_CURRENT_BINARY_DIR}/LibUSB-Win32/lib/gcc")
      SET(LIBUSB_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/LibUSB-Win32/bin/x86/")
    ELSE(MINGW)
      FIND_PATH(LIBUSB_INCLUDE_DIRS lusb0_usb.h "$ENV{ProgramW6432}/libusb-win32/include" NO_SYSTEM_ENVIRONMENT_PATH)
      FIND_LIBRARY(LIBUSB_LIBRARIES NAMES usb-1.0 libusb-1.0 libusb PATHS "$ENV{ProgramW6432}/libusb-win32/lib/msvc_x64")
      SET(LIBUSB_LIBRARY_DIR "$ENV{ProgramW6432}/libusb-win32/bin/amd64/")
    ENDIF(MINGW)
    # Must fix up variable to avoid backslashes during packaging
    STRING(REGEX REPLACE "\\\\" "/" LIBUSB_LIBRARY_DIR ${LIBUSB_LIBRARY_DIR})
  ELSE(WIN32)
    # If not under Windows we use PkgConfig
    FIND_PACKAGE (PkgConfig)
    IF(PKG_CONFIG_FOUND)
      PKG_CHECK_MODULES(LIBUSB REQUIRED libusb-1.0)
    ELSE(PKG_CONFIG_FOUND)
      MESSAGE(FATAL_ERROR "Could not find PkgConfig")
    ENDIF(PKG_CONFIG_FOUND)
  ENDIF(WIN32)
  
  IF(LIBUSB_INCLUDE_DIRS AND LIBUSB_LIBRARIES)
     SET(LIBUSB_FOUND TRUE)
  ENDIF(LIBUSB_INCLUDE_DIRS AND LIBUSB_LIBRARIES)
ENDIF(NOT LIBUSB_FOUND)

IF(LIBUSB_FOUND)
  IF(NOT LIBUSB_FIND_QUIETLY)
    MESSAGE(STATUS "Found LIBUSB: ${LIBUSB_LIBRARIES} ${LIBUSB_INCLUDE_DIRS}")
  ENDIF (NOT LIBUSB_FIND_QUIETLY)
ELSE(LIBUSB_FOUND)
  IF(LIBUSB_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find LIBUSB")
  ENDIF(LIBUSB_FIND_REQUIRED)
ENDIF(LIBUSB_FOUND)
