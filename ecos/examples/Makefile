# Mostly written by Jonathan Larmour, Red Hat, Inc.
# Reference to ecos.mak added by John Dallaway, eCosCentric Limited, 2003-01-20
# This file is in the public domain and may be used for any purpose

# Usage:   make INSTALL_DIR=/path/to/ecos/install

INSTALL_DIR=$$(INSTALL_DIR) # override on make command line

include $(INSTALL_DIR)/include/pkgconf/ecos.mak

XCC           = $(ECOS_COMMAND_PREFIX)gcc
XOBJCOPY      = $(ECOS_COMMAND_PREFIX)objcopy
XCXX          = $(XCC)
XLD           = $(XCC)

CFLAGS        = -I$(INSTALL_DIR)/include
CXXFLAGS      = $(CFLAGS)
LDFLAGS       = -nostartfiles -L$(INSTALL_DIR)/lib -Ttarget.ld

# RULES

.PHONY: all clean

all: hello twothreads simple-alarm serial flood ping_test sysctl1

clean:
	-rm -f hello hello.o twothreads twothreads.o flood.o ping_test sysctl1 sysctl1.o flood
	-rm -f simple-alarm simple-alarm.o serial serial.o 
	-rm -f instrument-test instrument-test.o
	-rm -f  *.bin
%.o: %.c
	$(XCC) -c -o $*.o $(CFLAGS) $(ECOS_GLOBAL_CFLAGS) $<

%.o: %.cxx
	$(XCXX) -c -o $*.o $(CXXFLAGS) $(ECOS_GLOBAL_CFLAGS) $<

%.o: %.C
	$(XCXX) -c -o $*.o $(CXXFLAGS) $(ECOS_GLOBAL_CFLAGS) $<

%.o: %.cc
	$(XCXX) -c -o $*.o $(CXXFLAGS) $(ECOS_GLOBAL_CFLAGS) $<

hello: hello.o
	$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $@ $@.o
	$(XOBJCOPY) -O binary $@ $@.bin

flood: flood.o
	$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $@ $@.o
	$(XOBJCOPY) -O binary $@ $@.bin

ping_test: ping_test.o
	$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $@ $@.o
	$(XOBJCOPY) -O binary $@ $@.bin

twothreads: twothreads.o
	$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $@ $@.o
	$(XOBJCOPY) -O binary $@ $@.bin

simple-alarm: simple-alarm.o
	$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $@ $@.o
	$(XOBJCOPY) -O binary $@ $@.bin

serial: serial.o
	$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $@ $@.o
	$(XOBJCOPY) -O binary $@ $@.bin

instrument-test: instrument-test.o
	$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $@ $@.o
	$(XOBJCOPY) -O binary $@ $@.bin

sysctl1: sysctl1.o
	$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $@ $@.o
	$(XOBJCOPY) -O binary $@ $@.bin
