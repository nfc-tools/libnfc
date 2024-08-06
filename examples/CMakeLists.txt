SET(EXAMPLES-SOURCES
  nfc-anticol
  nfc-dep-initiator
  nfc-dep-target
  nfc-emulate-forum-tag2
  nfc-emulate-tag
  nfc-emulate-uid
  nfc-mfsetuid
  nfc-poll
  nfc-relay
  nfc-st25tb
  pn53x-diagnose
  pn53x-sam
  pn53x-tamashell
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../libnfc)

# Examples
FOREACH(source ${EXAMPLES-SOURCES})
  SET (TARGETS ${source}.c)

  IF(WIN32)
    SET(RC_COMMENT "${PACKAGE_NAME} example")
    SET(RC_INTERNAL_NAME ${source})
    SET(RC_ORIGINAL_NAME ${source}.exe)
    SET(RC_FILE_TYPE VFT_APP)
    SET(RC_FILE_FLAGS 0x0)
    IF(LIBNFC_DEBUG_MODE)
      SET(RC_FILE_FLAGS VS_FF_DEBUG)
    ENDIF(LIBNFC_DEBUG_MODE)
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/../contrib/win32/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/../windows/${source}.rc @ONLY)
    LIST(APPEND TARGETS ${CMAKE_CURRENT_BINARY_DIR}/../windows/${source}.rc)

	IF(${source} MATCHES "nfc-st25tb")
		LIST(APPEND TARGETS ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/win32/getopt.c)
	ENDIF()
  ENDIF(WIN32)

  ADD_EXECUTABLE(${source} ${TARGETS})
  TARGET_LINK_LIBRARIES(${source} nfc)
  TARGET_LINK_LIBRARIES(${source} nfcutils)
  INSTALL(TARGETS ${source} RUNTIME DESTINATION bin COMPONENT examples)
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
