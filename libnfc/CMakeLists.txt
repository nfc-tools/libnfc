# Windows MinGW workarounds
IF(WIN32)
  SET(WINDOWS_SOURCES ../contrib/win32/stdlib.c)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../contrib/win32)

  # Add in the rc for version information in the dll
  LIST(APPEND WINDOWS_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/../windows/libnfc.rc)
  IF (NOT MINGW)
    LIST(APPEND WINDOWS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/win32/nfc_msvc.def)
  ENDIF()
ENDIF(WIN32)

# Library's chips
SET(CHIPS_SOURCES chips/pn53x.c)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/chips)

# Library's buses
IF(USB_REQUIRED)
  LIST(APPEND BUSES_SOURCES buses/usbbus.c)
ENDIF(USB_REQUIRED)

IF(UART_REQUIRED)
  IF(WIN32)
    # Windows have a special implementation for UART
    LIST(APPEND BUSES_SOURCES ../contrib/win32/libnfc/buses/uart.c)
  ELSE(WIN32)
    LIST(APPEND BUSES_SOURCES buses/uart.c)
  ENDIF(WIN32)
ENDIF(UART_REQUIRED)

IF(I2C_REQUIRED)
  IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    LIST(APPEND BUSES_SOURCES buses/i2c.c)
  ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Only Linux is supported at the moment
    #LIST(APPEND BUSES_SOURCES ../contrib/win32/libnfc/buses/i2c.c)
    MESSAGE( FATAL_ERROR "I2C is only (yet) supported in Linux!" )
  ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
ENDIF(I2C_REQUIRED)

IF(SPI_REQUIRED)
  IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    LIST(APPEND BUSES_SOURCES buses/spi.c)
  ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Only Linux is supported at the moment
    #LIST(APPEND BUSES_SOURCES ../contrib/win32/libnfc/buses/spi.c)
    MESSAGE( FATAL_ERROR "SPI is only (yet) supported in Linux!" )
  ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
ENDIF(SPI_REQUIRED)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/buses)

IF(PCSC_FOUND)
  INCLUDE_DIRECTORIES(${PCSC_INCLUDE_DIRS})
  LINK_DIRECTORIES(${PCSC_LIBRARY_DIRS})
ENDIF(PCSC_FOUND)

IF(LIBUSB_FOUND)
  INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIRS})
  LINK_DIRECTORIES(${LIBUSB_LIBRARY_DIRS})
ENDIF(LIBUSB_FOUND)

IF(LIBNFC_CONFFILES_MODE)
  SET(CONF_SOURCES conf.c)
ENDIF(LIBNFC_CONFFILES_MODE)

# Library
SET(LIBRARY_SOURCES nfc.c nfc-device.c nfc-emulation.c nfc-internal.c iso14443-subr.c mirror-subr.c target-subr.c ${DRIVERS_SOURCES} ${BUSES_SOURCES} ${CHIPS_SOURCES} ${WINDOWS_SOURCES} ${CONF_SOURCES})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

IF(LIBNFC_LOG)
  IF(WIN32)
    IF(MINGW)
      SET(CMAKE_C_FLAGS "-fgnu89-inline ${CMAKE_C_FLAGS}")
    ENDIF(MINGW)
    LIST(APPEND LIBRARY_SOURCES log.c ../contrib/win32/libnfc/log-internal.c)
  ELSE(WIN32)
    LIST(APPEND LIBRARY_SOURCES log.c log-internal.c)
  ENDIF(WIN32)
ENDIF(LIBNFC_LOG)
ADD_LIBRARY(nfc SHARED ${LIBRARY_SOURCES})

IF(PCSC_FOUND)
  TARGET_LINK_LIBRARIES(nfc ${PCSC_LIBRARIES})
ENDIF(PCSC_FOUND)

IF(LIBUSB_FOUND)
  TARGET_LINK_LIBRARIES(nfc ${LIBUSB_LIBRARIES})
ENDIF(LIBUSB_FOUND)

IF(LIBRT_FOUND)
  TARGET_LINK_LIBRARIES(nfc ${LIBRT_LIBRARIES})
ENDIF(LIBRT_FOUND)

SET_TARGET_PROPERTIES(nfc PROPERTIES SOVERSION 6 VERSION 6.0.0)

IF(WIN32)
  # Libraries that are windows specific
  TARGET_LINK_LIBRARIES(nfc wsock32)
  IF(MINGW)
    ADD_CUSTOM_COMMAND(
      OUTPUT libnfc.lib
      COMMAND ${DLLTOOL} -d ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/win32/nfc.def -l ${CMAKE_CURRENT_BINARY_DIR}/libnfc.lib ${CMAKE_CURRENT_BINARY_DIR}/libnfc.dll
      DEPENDS nfc ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/win32/nfc.def
    )
    ADD_CUSTOM_TARGET(win32lib ALL DEPENDS libnfc.lib)
  ELSE()
    ADD_LIBRARY(win32lib ALIAS nfc)
  ENDIF()

  # On Windows the shared (runtime) library should be either in the same 
  # directory as the excutables or in the path, we add it to same directory
  INSTALL(TARGETS nfc RUNTIME DESTINATION bin COMPONENT libraries)

  # At compile time we need the .LIB file, we place it in the lib directory
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/libnfc.lib DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)
ELSE(WIN32)
  INSTALL(TARGETS nfc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)
ENDIF(WIN32)

