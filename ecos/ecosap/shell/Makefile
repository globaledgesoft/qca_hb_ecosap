# Makefile for MIPS target



# Build tag
BUILD_TAG = echo `date` eCos@`hostname`

BUILD_VERSION_TAG = echo `git describe --tags --abbrev=0`

BUILD_VERSION_HASH = echo `git rev-parse --short HEAD && git rev-parse --short HEAD~1 && git rev-parse --short HEAD~2`

# Whoami
UNAME = `whoami | cut -c 1-2`
# Base target image name
TARGET_BASE_NAME = st

# eCos Target
ifeq ($(ECOS_INSTALL_DIR),)
$(error ECOS_INSTALL_DIR not set!)
endif
ECOS_INSTALL_DIR = $(ECOS_TARGET_DIR)/install
ECOS_INCLUDE_DIR = $(ECOS_INSTALL_DIR)/include
ECOS_LIB_DIR = $(ECOS_INSTALL_DIR)/lib

# GCC MIPS ELF target
ARCH = mipsisa32-elf-

# bintuils
CC = $(ARCH)gcc
CXX = $(ARCH)g++
AR = $(ARCH)ar
ARFLAGS = crv
LD = $(ARCH)ld

# Compiler release
COMPILER_RELEASE = echo `$(CC) --version | head -1`


include $(ECOS_INSTALL_DIR)/include/pkgconf/ecos.mak

# Optimization flags
OPT_FLAGS = -DSHELL_DEBUG

# Enable Optional Components
# TODO: Remove this from here and add to config file
OPT_FLAGS += -DECOS_NW_CLI

OPT_FLAGS += -DCONFIG_SHELL_NETMEMINFO
OPT_FLAGS += -DCONFIG_TRACK_MCLBUF

# Generic CFLAGS & LDFLAGS
CFLAGS += -Wall -Wno-unused $(DEBUG_FLAGS) $(OPT_FLAGS) $(CMD_FLAGS) -I. -I../include -ffunction-sections -fdata-sections $(ECOS_GLOBAL_CFLAGS)
LDFLAGS = $(DEBUG_FLAGS) -L$(ECOS_LIB_DIR) -Wl,--gc-sections -Ttarget.ld -nostdlib -msoft-float $(ECOS_GLOBAL_LDFLAGS)

# Flags for Speex
CFLAGS_NT = -Wall $(DEBUG_FLAGS) $(OPT_FLAGS) -I. -ffunction-sections -fdata-sections

# TCP/IP support
USE_TCPIP = 0

CFLAGS += -I$(ECOS_INCLUDE_DIR)
CFLAGS_NT += -I$(ECOS_INCLUDE_DIR)
LDFLAGS += -L$(ECOS_LIB_DIR)

# Phony targets
.PHONY = clean 

STATIC_LIB = lib$(NAME).a

.SUFFIXES: .o .c .cpp .cxx .s .S

CFLAGS +=	-DBUILD_TAG=\""$(shell $(BUILD_TAG))"\" -DCOMPILER_RELEASE=\""$(shell $(COMPILER_RELEASE))"\" \
		-DBUILD_VERSION_TAG=\""$(shell $(BUILD_VERSION_TAG))"\" \
		-DBUILD_VERSION_HASH=\""$(shell $(BUILD_VERSION_HASH))"\" \
		-DBUILD_FLAGS="\"$(DEBUG_FLAGS) $(OPT_FLAGS) "\" \

NAME = shelltask


all: $(NAME)

.c.o:
	$(CC) $(CFLAGS) -msoft-float -o $@ -c $<

.cpp.o:
	$(CXX) $(CFLAGS) -o $@ -c $<

$(NAME): $(OBJS)
	@echo $(OBJS) > /dev/null

.PHONY:	clean

clean:
ifeq ($(TARGET), library)
	rm -f $(OBJS) $(STATIC_LIB) *.o *~ 
else
	rm -f $(NAME) $(OBJS) $(STATIC_LIB) *.o *~ 
endif

# Dependencies
#
# Rebuild if Makefiles change

$(NAME):	Makefile
$(STATIC_LIB): Makefile
