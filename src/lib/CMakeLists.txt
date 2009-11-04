# Library's buses
SET(BUSES_SOURCES buses/uart)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/buses)

# Library's drivers
SET(DRIVERS_SOURCES drivers/arygon drivers/pn532_uart)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/drivers)

## find PCSC library and headers
IF(LIBNFC_PCSC)
  FIND_PACKAGE(PCSC REQUIRED)
  ADD_DEFINITIONS("-DHAVE_PCSC_LITE=1")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PCSC_CFLAGS_OTHER}")
  SET(DRIVERS_SOURCES ${DRIVERS_SOURCES} "drivers/acr122")
ENDIF(LIBNFC_PCSC)

## find libusb library and headers
IF(LIBNFC_USB)
  FIND_PACKAGE(LIBUSB REQUIRED)
  ADD_DEFINITIONS("-DHAVE_LIBUSB=1")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LIBUSB_CFLAGS_OTHER}")
  SET(DRIVERS_SOURCES ${DRIVERS_SOURCES} "drivers/pn531" "drivers/pn533")
ENDIF(LIBNFC_USB)


# Library
SET(LIBRARY_SOURCES nfc bitutils ${DRIVERS_SOURCES} ${BUSES_SOURCES})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ADD_LIBRARY(nfc SHARED ${LIBRARY_SOURCES})
TARGET_LINK_LIBRARIES(nfc ${LIBUSB_LIBRARIES} ${PCSC_LIBRARIES})
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


