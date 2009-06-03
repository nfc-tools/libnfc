###############################################
# Linux settings
###############################################
LIBPCSC_HEADERS=`pkg-config --cflags libpcsclite`
LIBPCSC_LIB=`pkg-config --libs libpcsclite`
LIBUSB_HEADERS=`pkg-config --cflags libusb`
LIBUSB_LIB=`pkg-config --libs libusb`
LIBNFC_TYPE=so
LIBNFC_CFLAGS=-shared
LIBNFC_LINK=-Wl,-rpath,.

###############################################
# General settings
###############################################
LIBNFC_LDFLAGS=$(LIBPCSC_LIB) $(LIBUSB_LIB)
LIBNFC_PATH=../../
VPATH=$(LIBNFC_PATH)

###############################################
# Compiler settings
###############################################
CC = gcc
LD = gcc
CFLAGS = -fPIC -Wall -O4 $(LIBPCSC_HEADERS) $(LIBUSB_HEADERS)
LDFLAGS = -fPIC -Wall -O4

OBJS = dev_pn531.o dev_pn533.o dev_acr122.o dev_arygon.o bitutils.o libnfc.o rs232.o
HEADERS = devices.h bitutils.h defines.h libnfc.h
LIBNFC = libnfc.$(LIBNFC_TYPE)
EXES = anticol emulate list mftool relay

all: $(LIBNFC) $(EXES)

libnfc.$(LIBNFC_TYPE): $(OBJS)
	$(LD) $(LDFLAGS) -o $(LIBNFC) $(LIBNFC_CFLAGS) $(LIBNFC_LDFLAGS) $(OBJS)

% : %.c $(LIBNFC)
	$(LD) $(LDFLAGS) -o $@ $< -L. -lnfc $(LIBNFC_LINK)

clean: 
	rm -f $(OBJS) $(LIBNFC) $(EXES)
