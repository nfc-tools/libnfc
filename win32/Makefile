###############################################
# General settings
###############################################
VPATH=../src

###############################################
# Compiler settings
###############################################
CC = cl
LD = link
CFLAGS = -O2 -GL -I "$(Include);$(INCLUDE)" -I "libusb" -I "stdint" -D "WIN32" -D "LIBNFC_EXPORTS" -MT -D "_CRT_SECURE_NO_WARNINGS" -D "HAVE_PCSC_LITE" -D "_WINDLL" -EHsc -W3 -nologo -c -Wp64 -TC -wd4996 -errorReport:prompt
LDFLAGS = -INCREMENTAL:NO -NOLOGO -LIBPATH:"libusb" -MANIFEST:NO -OPT:REF -OPT:ICF -LTCG -MACHINE:X86 -ERRORREPORT:PROMPT winscard.lib libusb.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

###############################################
# Files
###############################################
OBJS = dev_pn531.obj dev_pn533.obj dev_acr122.obj dev_arygon.obj bitutils.obj rs232.obj libnfc.obj
HEADERS = dev_acr122.h bitutils.h defines.h libnfc.h
LIBNFC = libnfc
EXES = anticol list emulate relay mftool

all: $(LIBNFC) $(EXES)

%.obj : %.c
	$(CC) $(CFLAGS) $<

libnfc: $(OBJS)
	$(LD) -OUT:$(LIBNFC).dll -DLL -DEF:"libnfc.def" -SUBSYSTEM:WINDOWS $(LDFLAGS) $(OBJS)

% : %.c $(LIBNFC)
	$(CC) $(CFLAGS) $< 
	$(LD) /OUT:nfc-$@.exe -SUBSYSTEM:CONSOLE  $(LDFLAGS) $(OBJS) $@

clean: 
	rm -f $(OBJS) $(LIBNFC) $(EXES)
