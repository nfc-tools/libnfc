###############################################
# General settings
###############################################
VPATH=../src

###############################################
# Compiler settings
###############################################
CC = cl
LD = link
CFLAGS = -O2 -GL -I "$(Include);$(INCLUDE)" -I "libusb" -I "stdint" -D "WIN32" -MT -D "_CRT_SECURE_NO_WARNINGS" -D "HAVE_PCSC_LITE" -D "HAVE_LIBUSB" -D "_WINDLL" -EHsc -W3 -nologo -TC -errorReport:prompt
LDFLAGS = -INCREMENTAL:NO -NOLOGO -LIBPATH:"libusb" -MANIFEST:NO -OPT:REF -OPT:ICF -LTCG libusb.lib libusb_x64.lib winscard.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

###############################################
# Files
###############################################
OBJS = dev_pn531.obj dev_pn533.obj dev_acr122.obj dev_arygon.obj bitutils.obj rs232.obj libnfc.obj
HEADERS = dev_acr122.h bitutils.h defines.h libnfc.h
LIBNFC = libnfc
EXES = nfc-anticol.exe nfc-list.exe nfc-emulate.exe nfc-relay.exe nfc-mftool.exe nfc-mfultool.exe nfc-initiator.exe nfc-target.exe

all: $(LIBNFC) $(EXES)

%.obj : %.c
	$(CC) -c $(CFLAGS) $<

libnfc:  $(OBJS)
	$(LD) $(LDFLAGS) -OUT:$(LIBNFC).dll -DLL -DEF:"libnfc.def" -SUBSYSTEM:WINDOWS $(OBJS)

nfc-%.exe : %.c $(LIBNFC)
	$(CC) $(CFLAGS) $< /link /OUT:$@ -SUBSYSTEM:CONSOLE $(LDFLAGS) $(LIBNFC).lib

clean: 
	del /Q $(OBJS) $(LIBNFC).dll $(LIBNFC).exp $(LIBNFC).lib *.obj $(EXES)