# This CMake script wants to use pcre functionality needed for windows 
# compilation. However, since PCRE isn't really a default install location
# there isn't much to search.
#
# Operating Systems Supported:
# - Windows (requires MinGW)
#   Tested with Windows XP/Windows 7
#
# This should work for both 32 bit and 64 bit systems.
#
# Author: A. Lian <alex.lian@gmail.com>
#

IF(WIN32)
  IF(NOT PCRE_FOUND)
    FIND_PATH(PCRE_INCLUDE_DIRS regex.h)
    FIND_LIBRARY(PCRE_LIBRARIES NAMES PCRE pcre)

    IF(PCRE_INCLUDE_DIRS AND PCRE_LIBRARIES)
       SET(PCRE_FOUND TRUE)
    ENDIF(PCRE_INCLUDE_DIRS AND PCRE_LIBRARIES)
  ENDIF(NOT PCRE_FOUND)

  IF(PCRE_FOUND)
    IF(NOT PCRE_FIND_QUIETLY)
      MESSAGE(STATUS "Found PCRE: ${PCRE_LIBRARIES} ${PCRE_INCLUDE_DIRS}")
    ENDIF (NOT PCRE_FIND_QUIETLY)
  ELSE(PCRE_FOUND)
    IF(PCRE_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find PCRE")
    ENDIF(PCRE_FIND_REQUIRED)
  ENDIF(PCRE_FOUND)

  INCLUDE(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCRE DEFAULT_MSG
    PCRE_LIBRARIES
    PCRE_INCLUDE_DIRS
  )

ENDIF(WIN32)
