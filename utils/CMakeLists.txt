SET(UTILS-SOURCES 
  nfc-emulate-forum-tag4
  nfc-list
  nfc-mfclassic
  nfc-mfultralight
  nfc-probe
  nfc-read-forum-tag3
  nfc-relay-picc
)

ADD_LIBRARY(nfcutils STATIC 
  nfc-utils.c
)

# Examples
FOREACH(source ${UTILS-SOURCES})
  IF((${source} MATCHES "nfc-mfultralight") OR (${source} MATCHES "nfc-mfclassic"))
    ADD_EXECUTABLE(${source} ${source}.c mifare)
  ELSE()
    ADD_EXECUTABLE(${source} ${source}.c)
  ENDIF((${source} MATCHES "nfc-mfultralight") OR (${source} MATCHES "nfc-mfclassic"))
  TARGET_LINK_LIBRARIES(${source} nfc)
  TARGET_LINK_LIBRARIES(${source} nfcutils)
  INSTALL(TARGETS ${source} RUNTIME DESTINATION bin COMPONENT utils)
ENDFOREACH(source)

#install required libraries
IF(WIN32)
  INCLUDE(InstallRequiredSystemLibraries)
  CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/FixBundle.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/FixBundle.cmake @ONLY)
  INSTALL(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/FixBundle.cmake)
ENDIF(WIN32)

IF(NOT WIN32)
  # Manuals for the examples
  FILE(GLOB manuals "${CMAKE_CURRENT_SOURCE_DIR}/*.1")
  INSTALL(FILES ${manuals} DESTINATION ${SHARE_INSTALL_PREFIX}/man/man1 COMPONENT manuals)
ENDIF(NOT WIN32)
