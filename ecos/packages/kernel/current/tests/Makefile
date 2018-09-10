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
ECOS_GLOBAL_CFLAGSx := $(patsubst -Wstrict-prototypes,,$(ECOS_GLOBAL_CFLAGS))
ECOS_GLOBAL_CFLAGSxx := $(patsubst -Woverloaded-virtual,,$(ECOS_GLOBAL_CFLAGSx))
ECOS_GLOBAL_CFLAGSxxx := $(patsubst -fno-rtti,,$(ECOS_GLOBAL_CFLAGSxx))
ECOS_GLOBAL_CFLAGS = $(ECOS_GLOBAL_CFLAGSxxx) 
CXXFLAGS      = $(CFLAGS) 
LDFLAGS       = -nostartfiles -L$(INSTALL_DIR)/lib -Ttarget.ld 

# RULES

.PHONY: all clean

_c=$(wildcard *.c)                                                              
_cc+=$(wildcard *.cc)                                                              
_C+=$(wildcard *.C)                                                              
_cxx+=$(wildcard *.cxx)                                                              

OBJS_ = $(filter %,$(_c:.c=) $(_C:.C=) $(_cc:.cc=) $(_cxx:.cxx=))
OBJS = $(filter %,$(_c:.c=.o) $(_C:.C=.o) $(_cc:.cc=.o) $(_cxx:.cxx=.o))
all: $(OBJS) 
#all: 
#	echo $(ECOS_GLOBAL_CFLAGS)
clean:
		-@rm *.o 
		-@rm $(OBJS_)
		-@rm *.bin
%.o: %.c 
	@$(XCC) -c -o $*.o $(CFLAGS) $(ECOS_GLOBAL_CFLAGS) $<
	@$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $(patsubst %.o,%,$@) $@
	@$(XOBJCOPY) -O binary $(patsubst %.o,%,$@) $(patsubst %.o,%,$@).bin                                          


%.o: %.cxx
	@$(XCXX) -c -o $*.o $(CXXFLAGS) $(ECOS_GLOBAL_CFLAGS) $<
	@$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $(patsubst %.o,%,$@) $@
	@$(XOBJCOPY) -O binary $(patsubst %.o,%,$@) $(patsubst %.o,%,$@).bin                                          

%.o: %.C
	@$(XCXX) -c -o $*.o $(CXXFLAGS) $(ECOS_GLOBAL_CFLAGS) $<
	@$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $(patsubst %.o,%,$@) $@
	@$(XOBJCOPY) -O binary $(patsubst %.o,%,$@) $(patsubst %.o,%,$@).bin                                          

%.o: %.cc
	@$(XCXX) -c -o $*.o $(CXXFLAGS) $(ECOS_GLOBAL_CFLAGS) $<
	@$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $(patsubst %.o,%,$@) $@
	@$(XOBJCOPY) -O binary $(patsubst %.o,%,$@) $(patsubst %.o,%,$@).bin                                          

