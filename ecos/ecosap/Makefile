#
# Copyright (c) 2018, Global Edge Software Ltd.
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Company mentioned in the Copyright nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER AND CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

ifeq ($(ECOS_REPOSITORY),)
        $(error ECOS_REPOSITORY is not exported  do 'export ECOS_REPOSITORY=/path-to-/ecos/packages/')
endif

uname_p := $(shell arch)
ifeq ($(uname_p),x86_64)
TARBALL = toolchain_mips_64.tar.bz
else
TARBALL = toolchain_mips_32.tar.bz
endif

ECOS_BUILD_DIR=$(ECOS_REPOSITORY)/../ecosap/_ecos_build_/

INSTALL_DIR=$(ECOS_BUILD_DIR)/install/

TOPDIR=$(ECOS_REPOSITORY)

TOOLBUILD_PATH=$(TOPDIR)/../../u-boot/build/

UBOOT_BUILD_PATH=$(TOPDIR)/../../u-boot/build/

ifeq ($(BUILD_TOOLCHAIN),1)
WHAT_TO_BUILD = toolchain_build 

ifeq ($(wildcard $(UBOOT_BUILD_PATH)/../boot/u-boot/tuboot.bin),)
WHAT_TO_BUILD += uboot
endif

endif
WHAT_TO_BUILD += kernel 


export PATH:=$(PATH):/opt/qca953x/qca953x_ecos/u-boot/build/gcc-4.3.3/build_mips/staging_dir/usr/bin

ifeq ($(wildcard $(ECOS_REPOSITORY)/../ecosap//.config),)
WHAT_TO_BUILD += menuconfig
endif

everything: $(WHAT_TO_BUILD) 
	make -f eCosap.mk
kernel:
	mkdir -p $(ECOS_BUILD_DIR) && cd $(ECOS_BUILD_DIR) 
ifeq ($(wildcard $(ECOS_BUILD_DIR)/ecos.ecc),)
	cd $(ECOS_BUILD_DIR) && ecosconfig new qca953x_mips32_4kc && ecosconfig tree 
endif
	cd $(ECOS_BUILD_DIR) && make -j4
	
menuconfig:
	make -f eCosap.mk menuconfig

toolchain_build:
	@if test ! -d /opt/qca953x; \
     	then su -c "mkdir /opt/qca953x/" && su -c "tar -xf $(TOOLBUILD_PATH)/$(TARBALL) -C /opt/qca953x";		\
	fi	

uboot:
	make -C $(UBOOT_BUILD_PATH) uboot BOARD_TYPE=board953x COMPRESSED_UBOOT=1

help:
	@echo make - will build eCos kernel and ecosap
	@echo make BUILD_TOOLCHAIN=1 - will build eCos kernel and ecosap toolchain tuboot and compressed ecosap
	@echo make everything BUILD_TOOLCHAIN=1 - does everything you ever wanted, toolchain,uboot,ecosap,compressed ecosap
	@echo make menuconfig - shows configuration menu to select shell commands
	@echo make toolchain_build - builds toolchain
	@echo make uboot      - build compressed uboot 
clean:
	@make -f eCosap.mk clean
	@make -C $(ECOS_BUILD_DIR) clean
	@rm -rf $(ECOS_BUILD_DIR)/ecos.ecc
distclean:
	@make -f eCosap.mk clean
	@rm -rf $(ECOS_BUILD_DIR)
