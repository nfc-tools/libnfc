###############################################
# General settings
###############################################
VERSION = 1.3.4
SOURCE = ../src
NFC_LIB = $(SOURCE)/lib
NFC_INCLUDE = $(SOURCE)/include
NFC_BUSES = $(NFC_LIB)/buses
VPATH = $(SOURCE)/examples $(NFC_LIB) $(NFC_BUSES) $(NFC_LIB)/chips $(NFC_LIB)/drivers 

###############################################
# Compiler settings
###############################################
CC = cl
LD = link
CFLAGS = -O2 -GL -I "$(Include);$(INCLUDE)" -I "$(NFC_INCLUDE)" -I "$(NFC_BUSES)" -I "stdint" -D "WIN32" -MT -D "_CRT_SECURE_NO_WARNINGS" -D "HAVE_PCSC_LITE" -D "PACKAGE_VERSION=\"$(VERSION)\"" -D "_WINDLL" -EHsc -W3 -nologo -TC -errorReport:prompt
LDFLAGS = -INCREMENTAL:NO -NOLOGO -MANIFEST:NO -OPT:REF -OPT:ICF -LTCG winscard.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

###############################################
# Files
###############################################
OBJS = pn53x.obj pn532_uart.obj acr122.obj arygon.obj bitutils.obj uart.obj nfc.obj
HEADERS = dev_acr122.h bitutils.h defines.h libnfc.h
LIBNFC = libnfc
EXES = nfc-anticol.exe nfc-list.exe nfc-emulate.exe nfc-relay.exe nfc-mfclassic.exe nfc-mfultralight.exe nfcip-initiator.exe nfcip-target.exe

all: $(LIBNFC) $(EXES)

%.obj : %.c
	$(CC) -c $(CFLAGS) $<

libnfc:  $(OBJS)
	$(LD) $(LDFLAGS) -OUT:$(LIBNFC).dll -DLL -DEF:"libnfc.def" -SUBSYSTEM:WINDOWS $(OBJS)

%.exe : %.c $(LIBNFC) 
	$(CC) $(CFLAGS) -I "$(NFC_LIB)" $< /link /OUT:$@ -SUBSYSTEM:CONSOLE $(LDFLAGS) $(LIBNFC).lib

clean: 
	del /Q $(OBJS) $(LIBNFC).dll $(LIBNFC).exp $(LIBNFC).lib *.obj $(EXES)
