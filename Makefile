###############################################
# Linux
###############################################
LIB_TYPE=so
LIB_FLAGS=-shared
PCSC_HEADERS=-I/usr/include/PCSC
PCSC_LIB=-lpcsclite -Wl,-rpath,.

###############################################
# MacOS X
############################################### 
#LIB_TYPE=dylib
#LIB_FLAGS=-dynamiclib -flat_namespace -undefined suppress
#PCSC_HEADERS=-I/System/Library/Frameworks/PCSC.framework/Headers
#PCSC_LIB=-framework PCSC

CC = gcc
LD = gcc
CFLAGS = -fPIC -Wall -std=c99 -O4 $(PCSC_HEADERS)
LDFLAGS = -fPIC $(PCSC_LIB)

OBJS = acr122.o bitutils.o libnfc.o
HEADERS = acr122.h bitutils.h defines.h libnfc.h
LIB = libnfc.$(LIB_TYPE)
EXES = anticol list simulate relay mfread mfwrite

all: $(LIB) $(EXES)

acr122.o: acr122.c $(HEADERS)
bitutils.o: bitutils.c $(HEADERS)
libnfc.o: libnfc.c $(HEADERS)

%.s : %.c
	$(CC) -S -dA $(CFLAGS) -o $@ $<

libnfc.$(LIB_TYPE): $(OBJS)
	$(LD) $(LDFLAGS) -o $(LIB) $(LIB_FLAGS) $(OBJS)

anticol: $(OBJS)
	$(LD) $(LDFLAGS) -o anticol anticol.c -L. -lnfc

list: $(OBJS)
	$(LD) $(LDFLAGS) -o list list.c -L. -lnfc

simulate: $(OBJS)
	$(LD) $(LDFLAGS) -o simulate simulate.c -L. -lnfc

relay: $(OBJS)
	$(LD) $(LDFLAGS) -o relay relay.c -L. -lnfc
 
mfread: $(OBJS)
	$(LD) $(LDFLAGS) -o mfread mfread.c -L. -lnfc
 
mfwrite: $(OBJS)
	$(LD) $(LDFLAGS) -o mfwrite mfwrite.c -L. -lnfc
 
clean: 
	rm -f $(OBJS) $(LIB) $(EXES)
