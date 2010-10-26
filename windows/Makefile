COMMONOPTS=/W3 /MD /DNDEBUG /DWIN32 /D_WIN32_DCOM /D_WIN32_WINNT=0x500 /D_CRT_SECURE_NO_DEPRECATE /Zp8 /DNTDDI_VERSION=0x05000400	

DLLNAME=nfc

NFCOPTS=	/I..\include				\
			/I..\contrib\win32			\
			/I..\contrib\win32\stdint	\
			/Iusb\include				\
			/I..\libnfc					\
			/I..\libnfc\buses			\
			/DDRIVER_PN531_USB_ENABLED	\
			/DDRIVER_PN533_USB_ENABLED	\
			/DDRIVER_ACR122_ENABLED		\
			/DDRIVER_ARYGON_ENABLED		\
			/DDRIVER_PN532_UART_ENABLED

NFCLIBOPTS=/Dnfc_EXPORTS /DPACKAGE_VERSION=\"1.4.0\"

CFLAGS=$(COMMONOPTS) /WX $(NFCOPTS)
CXXFLAGS=$(COMMONOPTS) /EHa $(NFCOPTS)
CC=cl.exe /nologo
CXX=$(CC)
CC_OUT_EXE=/Fe
CC_OUT_DLL=/LD /Fe
CC_OUT_OBJ=/Fo

LIBNFC_DLL=bin\$(DLLNAME).dll
NFC_LIST=bin\nfc-list.exe
NFC_ANTICOL=bin\nfc-anticol.exe
NFC_EMULATE=bin\nfc-emulate.exe
NFC_MFCLASSIC=bin\nfc-mfclassic.exe
NFC_MFULTRALIGHT=bin\nfc-mfultralight.exe
NFC_POLL=bin\nfc-poll.exe
NFC_RELAY=bin\nfc-relay.exe
NFCIP_INITIATOR=bin\nfcip-initiator.exe
NFCIP_TARGET=bin\nfcip-target.exe

LIBNFC_OBJ=	obj\nfc.obj				\
			obj\pn531_usb.obj		\
			obj\pn53x_usb.obj		\
			obj\usbstub.obj			\
			obj\uart.obj			\
			obj\pn53x.obj			\
			obj\mirror-subr.obj		\
			obj\iso14443-subr.obj	\
			obj\acr122.obj			\
			obj\arygon.obj			\
			obj\pn533_usb.obj		\
			obj\pn532_uart.obj

NFC_LIST_OBJ=obj\nfc-list.obj	\
			obj\nfc-utils.obj

NFC_POLL_OBJ=obj\nfc-poll.obj	\
			obj\nfc-utils.obj

NFC_RELAY_OBJ=obj\nfc-relay.obj	\
			obj\nfc-utils.obj

NFC_ANTICOL_OBJ=obj\nfc-anticol.obj	\
			obj\nfc-utils.obj

NFC_EMULATE_OBJ=obj\nfc-emulate.obj	\
			obj\nfc-utils.obj

NFCIP_INITIATOR_OBJ=obj\nfcip-initiator.obj	\
			obj\nfc-utils.obj

NFCIP_TARGET_OBJ=obj\nfcip-target.obj	\
			obj\nfc-utils.obj

NFC_MFCLASSIC_OBJ=obj\nfc-mfclassic.obj	\
			obj\mifare.obj \
			obj\nfc-utils.obj

NFC_MFULTRALIGHT_OBJ=obj\nfc-mfultralight.obj	\
			obj\mifare.obj \
			obj\nfc-utils.obj

all: obj bin $(LIBNFC_DLL) $(NFC_LIST) $(NFC_POLL) $(NFC_RELAY) $(NFCIP_INITIATOR) $(NFCIP_TARGET) $(NFC_ANTICOL) $(NFC_EMULATE) $(NFC_MFCLASSIC) $(NFC_MFULTRALIGHT)

clean:
	for %d in ( $(LIBNFC_DLL) $(NFC_LIST) $(NFC_POLL) $(NFC_RELAY) ) do if exist %d del %d
	for %d in ( $(LIBNFC_OBJ) $(NFC_LIST_OBJ) ) do if exist %d del %d
	for %d in ( obj\nfc-list.res obj\$(DLLNAME).res ) do if exist %d del %d
	for %d in ( bin\$(DLLNAME).exp bin\$(DLLNAME).lib obj\$(DLLNAME).lib ) do if exist %d del %d
	for %d in ( $(NFC_RELAY_OBJ) ) do if exist %d del %d
	for %d in ( $(NFC_POLL_OBJ) obj\nfc-poll.res ) do if exist %d del %d
	for %d in ( $(NFC_RELAY_OBJ) obj\nfc-relay.res ) do if exist %d del %d
	for %d in ( $(NFCIP_INITIATOR) $(NFCIP_INITIATOR_OBJ) obj\nfcip-initiator.res ) do if exist %d del %d
	for %d in ( $(NFCIP_TARGET) $(NFCIP_TARGET_OBJ) obj\nfcip-target.res ) do if exist %d del %d
	for %d in ( $(NFC_MFCLASSIC) $(NFC_MFCLASSIC_OBJ) obj\nfc-mfclassic.res ) do if exist %d del %d
	for %d in ( $(NFC_MFULTRALIGHT) $(NFC_MFULTRALIGHT_OBJ) obj\nfc-mfultralight.res ) do if exist %d del %d
	for %d in ( $(NFC_EMULATE) $(NFC_EMULATE_OBJ) obj\nfc-emulate.res ) do if exist %d del %d
	for %d in ( $(NFC_ANTICOL) $(NFC_ANTICOL_OBJ) obj\nfc-anticol.res ) do if exist %d del %d
	if exist obj rmdir obj
	if exist bin rmdir bin
	if exist mm\out rmdir /s /q mm\out 

obj bin:
	mkdir $@

obj\$(DLLNAME).lib: $(LIBNFC_OBJ)
	if exist $@ del $@
	lib /out:$@ $(LIBNFC_OBJ)

$(LIBNFC_DLL): obj\$(DLLNAME).lib win32\$(DLLNAME).def obj\$(DLLNAME).res
	$(CC) $(CC_OUT_DLL)$@			\
		obj\$(DLLNAME).lib			\
		obj\$(DLLNAME).res			\
		/link						\
		/DEF:win32\$(DLLNAME).def	\
		winscard.lib
	if exist $@.manifest mt.exe -manifest $@.manifest -outputresource:$@;#2
	if exist $@.manifest del $@.manifest

$(NFC_LIST): $(NFC_LIST_OBJ) $(LIBNFC_DLL) bin\$(DLLNAME).lib obj\nfc-list.res
	$(CC) $(CC_OUT_EXE)$@ $(NFC_LIST_OBJ) bin\$(DLLNAME).lib obj\nfc-list.res
	if exist $@.manifest mt.exe -manifest $@.manifest  -outputresource:$@;#1
	if exist $@.manifest del $@.manifest

$(NFC_EMULATE): $(NFC_EMULATE_OBJ) $(LIBNFC_DLL) bin\$(DLLNAME).lib obj\nfc-emulate.res
	$(CC) $(CC_OUT_EXE)$@ $(NFC_EMULATE_OBJ) bin\$(DLLNAME).lib obj\nfc-emulate.res
	if exist $@.manifest mt.exe -manifest $@.manifest  -outputresource:$@;#1
	if exist $@.manifest del $@.manifest

$(NFC_POLL): $(NFC_POLL_OBJ) $(LIBNFC_DLL) bin\$(DLLNAME).lib obj\nfc-poll.res
	$(CC) $(CC_OUT_EXE)$@ $(NFC_POLL_OBJ) bin\$(DLLNAME).lib obj\nfc-poll.res
	if exist $@.manifest mt.exe -manifest $@.manifest  -outputresource:$@;#1
	if exist $@.manifest del $@.manifest

$(NFC_ANTICOL): $(NFC_ANTICOL_OBJ) $(LIBNFC_DLL) bin\$(DLLNAME).lib obj\nfc-anticol.res
	$(CC) $(CC_OUT_EXE)$@ $(NFC_ANTICOL_OBJ) bin\$(DLLNAME).lib obj\nfc-anticol.res
	if exist $@.manifest mt.exe -manifest $@.manifest  -outputresource:$@;#1
	if exist $@.manifest del $@.manifest

$(NFC_RELAY): $(NFC_RELAY_OBJ) $(LIBNFC_DLL) bin\$(DLLNAME).lib obj\nfc-relay.res
	$(CC) $(CC_OUT_EXE)$@ $(NFC_RELAY_OBJ) bin\$(DLLNAME).lib obj\nfc-relay.res
	if exist $@.manifest mt.exe -manifest $@.manifest  -outputresource:$@;#1
	if exist $@.manifest del $@.manifest

$(NFCIP_INITIATOR): $(NFCIP_INITIATOR_OBJ) $(LIBNFC_DLL) bin\$(DLLNAME).lib obj\nfcip-initiator.res
	$(CC) $(CC_OUT_EXE)$@ $(NFCIP_INITIATOR_OBJ) bin\$(DLLNAME).lib obj\nfcip-initiator.res
	if exist $@.manifest mt.exe -manifest $@.manifest  -outputresource:$@;#1
	if exist $@.manifest del $@.manifest

$(NFCIP_TARGET): $(NFCIP_TARGET_OBJ) $(LIBNFC_DLL) bin\$(DLLNAME).lib obj\nfcip-target.res
	$(CC) $(CC_OUT_EXE)$@ $(NFCIP_TARGET_OBJ) bin\$(DLLNAME).lib obj\nfcip-target.res
	if exist $@.manifest mt.exe -manifest $@.manifest  -outputresource:$@;#1
	if exist $@.manifest del $@.manifest

$(NFC_MFCLASSIC): $(NFC_MFCLASSIC_OBJ) $(LIBNFC_DLL) bin\$(DLLNAME).lib obj\nfc-mfclassic.res
	$(CC) $(CC_OUT_EXE)$@ $(NFC_MFCLASSIC_OBJ) bin\$(DLLNAME).lib obj\nfc-mfclassic.res
	if exist $@.manifest mt.exe -manifest $@.manifest  -outputresource:$@;#1
	if exist $@.manifest del $@.manifest

$(NFC_MFULTRALIGHT): $(NFC_MFULTRALIGHT_OBJ) $(LIBNFC_DLL) bin\$(DLLNAME).lib obj\nfc-mfultralight.res
	$(CC) $(CC_OUT_EXE)$@ $(NFC_MFULTRALIGHT_OBJ) bin\$(DLLNAME).lib obj\nfc-mfultralight.res
	if exist $@.manifest mt.exe -manifest $@.manifest  -outputresource:$@;#1
	if exist $@.manifest del $@.manifest

obj\mifare.obj: ..\examples\mifare.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\mifare.c

obj\nfc-relay.obj: ..\examples\nfc-relay.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\nfc-relay.c

obj\nfc-mfclassic.obj: ..\examples\nfc-mfclassic.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\nfc-mfclassic.c

obj\nfc-mfultralight.obj: ..\examples\nfc-mfultralight.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\nfc-mfultralight.c

obj\nfc-emulate.obj: ..\examples\nfc-emulate.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\nfc-emulate.c

obj\nfcip-initiator.obj: ..\examples\nfcip-initiator.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\nfcip-initiator.c

obj\nfcip-target.obj: ..\examples\nfcip-target.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\nfcip-target.c

obj\nfc-poll.obj: ..\examples\nfc-poll.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\nfc-poll.c

obj\nfc-anticol.obj: ..\examples\nfc-anticol.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\nfc-anticol.c

obj\nfc-list.obj: ..\examples\nfc-list.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\nfc-list.c

obj\nfc-utils.obj: ..\examples\nfc-utils.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(CFLAGS) ..\examples\nfc-utils.c

obj\nfc.obj: ..\libnfc\nfc.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\nfc.c

obj\iso14443-subr.obj: ..\libnfc\iso14443-subr.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\iso14443-subr.c

obj\pn531_usb.obj: ..\libnfc\drivers\pn531_usb.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\drivers\pn531_usb.c

obj\pn533_usb.obj: ..\libnfc\drivers\pn533_usb.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\drivers\pn533_usb.c

obj\pn532_uart.obj: ..\libnfc\drivers\pn532_uart.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\drivers\pn532_uart.c

obj\pn53x_usb.obj: ..\libnfc\drivers\pn53x_usb.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\drivers\pn53x_usb.c

obj\acr122.obj: ..\libnfc\drivers\acr122.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\drivers\acr122.c

obj\arygon.obj: ..\libnfc\drivers\arygon.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\drivers\arygon.c

obj\pn53x.obj: ..\libnfc\chips\pn53x.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\chips\pn53x.c

obj\uart.obj: ..\libnfc\buses\uart.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\buses\uart.c

obj\usbstub.obj: usb\src\usbstub.cpp
	$(CXX) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CXXFLAGS) usb\src\usbstub.cpp

obj\mirror-subr.obj: ..\libnfc\mirror-subr.c
	$(CC) /c $(CC_OUT_OBJ)$@ $(NFCLIBOPTS) $(CFLAGS) ..\libnfc\mirror-subr.c

obj\$(DLLNAME).res: win32\$(DLLNAME).rc
	rc /r $(RCFLAGS) /fo$@ win32\$(DLLNAME).rc

obj\nfc-list.res: win32\nfc-list.rc
	rc /r $(RCFLAGS) /fo$@ win32\nfc-list.rc

obj\nfc-poll.res: win32\nfc-poll.rc
	rc /r $(RCFLAGS) /fo$@ win32\nfc-poll.rc

obj\nfc-relay.res: win32\nfc-relay.rc
	rc /r $(RCFLAGS) /fo$@ win32\nfc-relay.rc

obj\nfcip-initiator.res: win32\nfcip-initiator.rc
	rc /r $(RCFLAGS) /fo$@ win32\nfcip-initiator.rc

obj\nfcip-target.res: win32\nfcip-target.rc
	rc /r $(RCFLAGS) /fo$@ win32\nfcip-target.rc

obj\nfc-anticol.res: win32\nfc-anticol.rc
	rc /r $(RCFLAGS) /fo$@ win32\nfc-anticol.rc

obj\nfc-emulate.res: win32\nfc-emulate.rc
	rc /r $(RCFLAGS) /fo$@ win32\nfc-emulate.rc

obj\nfc-mfclassic.res: win32\nfc-mfclassic.rc
	rc /r $(RCFLAGS) /fo$@ win32\nfc-mfclassic.rc

obj\nfc-mfultralight.res: win32\nfc-mfultralight.rc
	rc /r $(RCFLAGS) /fo$@ win32\nfc-mfultralight.rc

install: all
	cd mm
	call make.bat
	cd ..


	
