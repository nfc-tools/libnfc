SET(EXAMPLES-SOURCES nfc-list nfc-mfclassic nfc-mfultralight nfcip-initiator nfcip-target nfc-anticol nfc-emulate nfc-relay)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../lib)

# Examples
FOREACH(source ${EXAMPLES-SOURCES})
  ADD_EXECUTABLE(${source} ${source}.c)
  TARGET_LINK_LIBRARIES(${source} nfc)
  INSTALL(TARGETS ${source} RUNTIME DESTINATION bin COMPONENT examples)
ENDFOREACH(source)

# Manuals for the examples
FILE(GLOB manuals "${CMAKE_CURRENT_SOURCE_DIR}/*.1")
INSTALL(FILES ${manuals} DESTINATION ${SHARE_INSTALL_PREFIX}/man/man1 COMPONENT manuals)
