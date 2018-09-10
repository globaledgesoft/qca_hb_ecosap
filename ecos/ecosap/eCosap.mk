# Mostly written by Jonathan Larmour, Red Hat, Inc.
# Reference to ecos.mak added by John Dallaway, eCosCentric Limited, 2003-01-20
# This file is in the public domain and may be used for any purpose

#Usage: You don't have to use this makefile at all.
#there is one more file named 'makefile' use that

ifneq ($(wildcard $(ECOS_REPOSITORY)/../ecosap//.config),)
include  $(ECOS_REPOSITORY)/../ecosap//.config 
endif

ECOS_BUILD_DIR=$(ECOS_REPOSITORY)/../ecosap/_ecos_build_/

INSTALL_DIR=$(ECOS_BUILD_DIR)/install

TOPDIR=$(ECOS_REPOSITORY)

TOOLBUILD_PATH=$(TOPDIR)/../../u-boot/build/

UBOOT_BUILD_PATH=$(TOPDIR)/../../u-boot/build/

LZMA_PATH=$(TOPDIR)/../../u-boot/build/util/
MKIMAGE_PATH=$(TOPDIR)/../../u-boot/boot/u-boot/tools/
#Above tools will be generated once the u-boot is built

include $(INSTALL_DIR)/include/pkgconf/ecos.mak

XCC           = $(ECOS_COMMAND_PREFIX)gcc
XOBJCOPY      = $(ECOS_COMMAND_PREFIX)objcopy
XCXX          = $(XCC)
XLD           = $(XCC)

CFLAGS        = -I$(INSTALL_DIR)/include -Iinclude/
CXXFLAGS      = $(CFLAGS)
LDFLAGS       = -nostartfiles -L$(INSTALL_DIR)/lib -Ttarget.ld

SRC=$(wildcard *.c)

SHELL_SRC:=$(wildcard shell/*.c)

MCONF_DIR :=$(TOPDIR)/../host/tools/menuconfig
UBOOT_DIR :=$(TOPDIR)/../../u-boot/
CONFIG    :=$(UBOOT_DIR)/build/gcc-4.3.3/
OBJ = $(SRC:.c=.o)
HOSTCC    :=`which gcc`
# RULES

ifneq ($(wildcard $(LZMA_PATH)/lzma),)
LZMA_SUPPORT = ecosap.lzma.uImage
endif




ifdef CONFIG_BSP_PACKAGE
BSP_OBJS= bsp_commands.o
# TODO: Need to do it in better way
ifdef CONFIG_BSP_DATE
CMD_FLAGS += -DCONFIG_BSP_DATE
endif
ifdef CONFIG_BSP_UPTIME
CMD_FLAGS += -DCONFIG_BSP_UPTIME
endif
ifdef CONFIG_BSP_GPIO
CMD_FLAGS += -DCONFIG_BSP_GPIO
endif
ifdef CONFIG_BSP_FLASH_API
CMD_FLAGS += -DCONFIG_BSP_FLASH_API
endif
ifdef CONFIG_BSP_UPGRADE
CMD_FLAGS += -DCONFIG_BSP_UPGRADE
endif
ifdef CONFIG_BSP_EXCEPTION
CMD_FLAGS += -DCONFIG_BSP_EXCEPTION
endif
ifdef CONFIG_BSP_WD
CMD_FLAGS += -DCONFIG_BSP_WD
endif
ifdef CONFIG_BSP_SORT
CMD_FLAGS += -DCONFIG_BSP_SORT
endif
endif

NET_OBJS = cli_utils.o set_lan_wan_util.o ping.o

ifdef CONFIG_NET_PACKAGE
NET_OBJS += net_commands.o
endif

ifdef CONFIG_WLAN_PACKAGE
WLAN_OBJS = wlan_commands.o
ifdef CONFIG_WLAN_WLANCONFIG
CMD_FLAGS += -DCONFIG_WLAN_WLANCONFIG
endif
ifdef CONFIG_WLAN_HOSTAPD
CMD_FLAGS += -DCONFIG_WLAN_HOSTAPD
endif
ifdef CONFIG_WLAN_WPASUPPLICANT
CMD_FLAGS += -DCONFIG_WLAN_WPASUPPLICANT
endif
ifdef CONFIG_WLAN_IWCONFIG
CMD_FLAGS += -DCONFIG_WLAN_IWCONFIG
endif
ifdef CONFIG_WLAN_IWPRIV
CMD_FLAGS += -DCONFIG_WLAN_IWPRIV
endif
ifdef CONFIG_WLAN_ACFG
CMD_FLAGS += -DCONFIG_WLAN_ACFG
endif
endif

OBJS = buildtag.o \
        builtin_commands.o \
        init.o \
        shell_thread.o \
        shelltask.o \
        thread_cleanup.o \
        thread_info.o \
        misc.o   \
        strtoul.o       \
        ftpclient.o	\
        getopt.o        \
        getopt_long.o   \
        ethreg.o        \
        ifconfig_cmd.o \
     	cpuload.o	\
	    flash.o	       \
        $(WLAN_OBJS)  \
        $(NET_OBJS)   \
	    $(BSP_OBJS)

.PHONY: all clean shell

all: shell ecosap $(LZMA_SUPPORT)

ecosap.lzma.uImage:ecosap ecosap.bin
	$(LZMA_PATH)/lzma e ecosap.bin ecosap.lzma -lp2 -mfbt3
	$(MKIMAGE_PATH)/mkimage -A mips -O eCos -T kernel -C lzma -e 0x800001bc -a 0x80000000 -n "eCosAP" -d ecosap.lzma $@
	
OBJS2 = $(addprefix shell/, $(OBJS)) 


shell: $(SHELL_SRC)
ifeq ($(ECOS_REPOSITORY)/../ecosap/shell/.config,)
	$(error .config not found  do 'make INSTALL_DIR=/path/to/ecos/install menuconfig')
endif
	@$(MAKE) CMD_FLAGS="$(CMD_FLAGS)" OBJS="$(OBJS)" ECOS_INSTALL_DIR=$(INSTALL_DIR) -C shell
	-rm -f *.o ecosap.bin ecosap

%.o: %.c 
	$(XCC) -c -o $@ $(CFLAGS) $(CMD_FLAGS) $(ECOS_GLOBAL_CFLAGS) $<

ecosap: $(OBJ)
	$(XLD) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) $(OBJ) $(OBJS2) -o $@
	$(XOBJCOPY) -O binary $@ $@.bin

$(MCONF_DIR)/mconf:
	mkdir -p $(MCONF_DIR)/build	
	cd $(MCONF_DIR)/build && cmake ../
	$(MAKE) -C $(MCONF_DIR)/build

defconfig:  $(MCONF_DIR)/mconf configs/ecosap_defConfig.in
	  $(MCONF_DIR)/build/mconf configs/ecosap_defConfig.in
	  $(MAKE) ECOS_INSTALL_DIR=$(INSTALL_DIR) clean -C shell
	  -rm -f ecosap.lzma.uImage ecosap.lzma
	  -rm -f *.o *.bin ecosap

menuconfig: $(MCONF_DIR)/mconf Config.in
	$(MCONF_DIR)/build/mconf Config.in
	$(MAKE) ECOS_INSTALL_DIR=$(INSTALL_DIR) clean -C shell
	-rm -f ecosap.lzma.uImage ecosap.lzma
	-rm -f *.o *.bin ecosap

kernelconfig: $(INSTALL_DIR)/../ecos.ecc
	configtool_ecos $(ECOS_REPOSITORY) $(INSTALL_DIR)/../ecos.ecc 
	cd $(INSTALL_DIR)/../ && ecosconfig tree

kernelclean:
	cd $(INSTALL_DIR)/../ && ecosconfig tree
	cd $(INSTALL_DIR)/../ && make clean && rm -f install/lib/*

clean:
	-rm -f ecosap.lzma.uImage ecosap.lzma
	-rm -f *.o *.bin ecosap
	$(MAKE) ECOS_INSTALL_DIR=$(INSTALL_DIR) clean -C shell
	-rm -rf $(MCONF_DIR)/build/
