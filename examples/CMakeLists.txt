SET(EXAMPLES-SOURCES nfc-anticol nfc-dep-initiator nfc-dep-target nfc-emulate-forum-tag4 nfc-emulate-tag nfc-emulate-uid nfc-list nfc-poll nfc-relay nfc-relay-picc)
#TODO nfc-mfclassic nfc-mfultralight pn53x-diagnose pn53x-sam pn53x-tamashell

# XXX: Examples should not use private API!
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../libnfc)

INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIRS} ${PCSC_INCLUDE_DIRS})
LINK_DIRECTORIES(${LIBUSB_LIBRARY_DIRS} ${PCSC_LIBRARY_DIRS})

ADD_LIBRARY(nfc-utils nfc-utils)
#ADD_LIBRARY(mifare mifare)

# Examples
FOREACH(source ${EXAMPLES-SOURCES})
  ADD_EXECUTABLE(${source} ${source}.c)
  TARGET_LINK_LIBRARIES(${source} nfc)
  TARGET_LINK_LIBRARIES(${source} nfc-utils)
  INSTALL(TARGETS ${source} RUNTIME DESTINATION bin COMPONENT examples)
ENDFOREACH(source)

#TARGET_LINK_LIBRARIES(nfc-mfclassic mifare)
#TARGET_LINK_LIBRARIES(nfc-mfultralight mifare)

IF(NOT WIN32)
  # Manuals for the examples
  FILE(GLOB manuals "${CMAKE_CURRENT_SOURCE_DIR}/*.1")
  INSTALL(FILES ${manuals} DESTINATION ${SHARE_INSTALL_PREFIX}/man/man1 COMPONENT manuals)
ENDIF(NOT WIN32)
