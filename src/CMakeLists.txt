SET(LIBRARY-SOURCES libnfc dev_pn531 dev_pn533 rs232 bitutils dev_arygon)
SET(TOOLS-SOURCES list mftool mfultool initiator target)

# disabled for now on Windows, due to lacking getopt
IF(NOT MSVC)
  SET(TOOLS-SOURCES ${TOOLS-SOURCES} anticol emulate relay)
ENDIF(NOT MSVC)

IF(LIBNFC_PCSC)
  ADD_DEFINITIONS("-DHAVE_PCSC_LITE=1")
  SET(LIBRARY-SOURCES ${LIBRARY-SOURCES} "dev_acr122")
ENDIF(LIBNFC_PCSC)

IF(LIBNFC_VERBOSE_OUTPUT)
  ADD_DEFINITIONS("-DDEBUG")
ENDIF(LIBNFC_VERBOSE_OUTPUT)

IF(LIBNFC_DISABLE_SERIAL_AUTOPROBE)
  ADD_DEFINITIONS("-DDISABLE_SERIAL_AUTOPROBE")
ENDIF(LIBNFC_DISABLE_SERIAL_AUTOPROBE)

IF(LIBNFC_LANG_C99 AND NOT MSVC)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
ENDIF(LIBNFC_LANG_C99 AND NOT MSVC)

IF(MSVC)
  # Hide some warnings, this should be fixed in the code instead!
  ADD_DEFINITIONS("-D_CRT_SECURE_NO_WARNINGS")
ENDIF(MSVC)

IF(MSVC)
  # Include the stdint headers, because MSVC does not have them
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../win32/stdint)

  # This is from libusb-win32.sourceforge.net
  SET(LIBUSB_INCLUDE_DIRS $ENV{ProgramFiles}/LibUSB-Win32/include)
  SET(LIBUSB_LIBRARIES $ENV{ProgramFiles}/LibUSB-Win32/lib/msvc/libusb.lib)

  IF(LIBNFC_PCSC)
    # This is included with Visual Studio Express 2008, we should really use 
    # something that finds this library, whatever the MSVC version
    SET(LIBPCSCLITE_LIBRARIES "$ENV{ProgramFiles}/Microsoft SDKs/Windows/v6.0A/Lib/WinSCard.Lib")
    # On MSVC we don't need the winscard.h header in the include path because
    # it already is there...
  ENDIF(LIBNFC_PCSC)
ELSE(MSVC)
  # On Unix we just use pkg-config
  PKG_CHECK_MODULES(LIBUSB      REQUIRED libusb)
  IF(LIBNFC_PCSC)
    PKG_CHECK_MODULES(LIBPCSCLITE REQUIRED libpcsclite)
  ENDIF(LIBNFC_PCSC)
ENDIF(MSVC)

INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIRS} ${LIBPCSCLITE_INCLUDE_DIRS})

# Library
ADD_LIBRARY(nfc SHARED ${LIBRARY-SOURCES})
TARGET_LINK_LIBRARIES(nfc ${LIBUSB_LIBRARIES} ${LIBPCSCLITE_LIBRARIES})
SET_TARGET_PROPERTIES(nfc PROPERTIES SOVERSION 0 VERSION 0.0.0)

IF(MSVC)
  # On Windows the shared (runtime) library should be either in the same 
  # directory as the excutables or in the path, we add it to same directory
  INSTALL(TARGETS nfc RUNTIME DESTINATION bin COMPONENT libraries)

  # At compile time we need the .LIB file, we place it in the lib directory
  INSTALL(TARGETS nfc ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT headers)
ELSE(MSVC)
  INSTALL(TARGETS nfc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)
ENDIF(MSVC)

# Headers
FILE(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
INSTALL(FILES ${headers} DESTINATION ${INCLUDE_INSTALL_DIR}/libnfc COMPONENT headers)
IF(MSVC)
  # On Windows we also install the stdint headers, without it programs using it
  # can't compile (like if we want to compile the included examples "out of 
  # tree")
  #FILE(GLOB stdint "${CMAKE_CURRENT_SOURCE_DIR}/msvc/*.h")
  #INSTALL(FILES ${stdint} DESTINATION ${INCLUDE_INSTALL_DIR}/libnfc COMPONENT headers)
ENDIF(MSVC)

# Examples
FOREACH(source ${TOOLS-SOURCES})
  ADD_EXECUTABLE(nfc-${source} ${source}.c)
  TARGET_LINK_LIBRARIES(nfc-${source} nfc)
  INSTALL(TARGETS nfc-${source} RUNTIME DESTINATION bin COMPONENT examples)
ENDFOREACH(source)

# Manuals for the examples
FILE(GLOB manuals "${CMAKE_CURRENT_SOURCE_DIR}/*.1")
INSTALL(FILES ${manuals} DESTINATION ${SHARE_INSTALL_PREFIX}/man/man1 COMPONENT manuals)
