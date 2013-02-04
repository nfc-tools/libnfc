# Windows MinGW workarounds
IF(WIN32)
  SET(WINDOWS_SOURCES ../contrib/win32/stdlib)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../contrib/win32)

  # Add in the rc for version information in the dll
  LIST(APPEND WINDOWS_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/../windows/libnfc.rc)
ENDIF(WIN32)

# Library's chips
SET(CHIPS_SOURCES chips/pn53x)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/chips)

# Library's buses
SET(BUSES_SOURCES buses/uart)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/buses)

INCLUDE(LibnfcDrivers)

IF(WIN32)
  # Windows now requires regex, so we utilize PCRE
  # since Windows doesn't get the benefit of finding in CMake
  # it has to be added manually
  IF(PCRE_FOUND)
    INCLUDE_DIRECTORIES(${PCRE_INCLUDE_DIRS})
    LINK_DIRECTORIES(${PCRE_LIBRARY_DIRS})
  ENDIF(PCRE_FOUND)
ENDIF(WIN32)

IF(PCSC_FOUND)
  INCLUDE_DIRECTORIES(${PCSC_INCLUDE_DIRS})
  LINK_DIRECTORIES(${PCSC_LIBRARY_DIRS})
ENDIF(PCSC_FOUND)

IF(LIBUSB_FOUND)
  INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIRS})
  LINK_DIRECTORIES(${LIBUSB_LIBRARY_DIRS})
ENDIF(LIBUSB_FOUND)

# Library
SET(LIBRARY_SOURCES nfc nfc-device nfc-emulation nfc-internal conf iso14443-subr mirror-subr target-subr log ${DRIVERS_SOURCES} ${BUSES_SOURCES} ${CHIPS_SOURCES} ${WINDOWS_SOURCES})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

IF(LIBNFC_LOG)
  LIST(APPEND LIBRARY_SOURCES log-printf)
ENDIF(LIBNFC_LOG)
ADD_LIBRARY(nfc SHARED ${LIBRARY_SOURCES})

IF(PCSC_FOUND)
  TARGET_LINK_LIBRARIES(nfc ${PCSC_LIBRARIES})
ENDIF(PCSC_FOUND)

IF(LIBUSB_FOUND)
  TARGET_LINK_LIBRARIES(nfc ${LIBUSB_LIBRARIES})
ENDIF(LIBUSB_FOUND)

SET_TARGET_PROPERTIES(nfc PROPERTIES SOVERSION 0)

IF(WIN32)
  # Libraries that are windows specific
  TARGET_LINK_LIBRARIES(nfc wsock32)
  IF(PCRE_FOUND)
    TARGET_LINK_LIBRARIES(nfc ${PCRE_LIBRARIES})
  ENDIF(PCRE_FOUND)

  ADD_CUSTOM_COMMAND(
    OUTPUT libnfc.lib
    COMMAND dlltool -d ${CMAKE_CURRENT_SOURCE_DIR}/../windows/win32/nfc.def -l ${CMAKE_CURRENT_BINARY_DIR}/libnfc.lib ${CMAKE_CURRENT_BINARY_DIR}/libnfc.dll
    DEPENDS nfc ${CMAKE_CURRENT_SOURCE_DIR}/../windows/win32/nfc.def
  )
  ADD_CUSTOM_TARGET(win32lib ALL DEPENDS libnfc.lib)

  # On Windows the shared (runtime) library should be either in the same 
  # directory as the excutables or in the path, we add it to same directory
  INSTALL(TARGETS nfc RUNTIME DESTINATION bin COMPONENT libraries)

  # At compile time we need the .LIB file, we place it in the lib directory
  INSTALL(TARGETS nfc ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT headers)
ELSE(WIN32)
  INSTALL(TARGETS nfc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)
ENDIF(WIN32)

