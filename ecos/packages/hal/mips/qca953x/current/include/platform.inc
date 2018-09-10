#ifndef CYGONCE_HAL_PLATFORM_INC
#define CYGONCE_HAL_PLATFORM_INC
##=============================================================================
##
##	platform.inc
##
##	Malta board assembler header file
##
##=============================================================================
## ####ECOSGPLCOPYRIGHTBEGIN####
## -------------------------------------------
## This file is part of eCos, the Embedded Configurable Operating System.
## Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
##
## eCos is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free
## Software Foundation; either version 2 or (at your option) any later
## version.
##
## eCos is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
## for more details.
##
## You should have received a copy of the GNU General Public License
## along with eCos; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
##
## As a special exception, if other files instantiate templates or use
## macros or inline functions from this file, or you compile this file
## and link it with other works to produce a work based on this file,
## this file does not by itself cause the resulting work to be covered by
## the GNU General Public License. However the source code for this file
## must still be made available in accordance with section (3) of the GNU
## General Public License v2.
##
## This exception does not invalidate any other reasons why a work based
## on this file might be covered by the GNU General Public License.
## -------------------------------------------
## ####ECOSGPLCOPYRIGHTEND####
##=============================================================================
#######DESCRIPTIONBEGIN####
##
## Author(s): 	dmoseley
## Contributors:dmoseley, jskov
## Date:	2001-03-20
## Purpose:	Malta board definitions.
## Description:	This file contains various definitions and macros that are
##              useful for writing assembly code for the Malta board.
## Usage:
##		#include <cyg/hal/platform.inc>
##		...
##
##
######DESCRIPTIONEND####
##
##=============================================================================

#include <cyg/hal/mips.inc>

#include <cyg/hal/hal_arch.h>
#include <cyg/hal/plf_io.h>

#------------------------------------------------------------------------------
# Additional bits for status register.
# We set the IM[0] bit to accept all interrupts.

#define INITIAL_SR_PLF 0x00000400

#------------------------------------------------------------------------------

#define CYGPKG_HAL_RESET_VECTOR_FIRST_CODE
	.macro hal_reset_vector_first_code
	.endm

#define CYGPKG_HAL_EARLY_INIT
	.macro	hal_early_init
	.endm

#------------------------------------------------------------------------------
# Interrupt decode macros

# Only INTN[0] is connected on the Malta board. We need to access the
# interrupt controller to get the actual vector number.

#ifndef CYGPKG_HAL_MIPS_INTC_INIT_DEFINED

	.macro	hal_intc_init
	mfc0	v0,status
	nop
	ori	v0,v0,0xff00	# Enable interrupts
	mtc0	v0,status
	nop
	nop
	nop
	.endm

#define CYGPKG_HAL_MIPS_INTC_INIT_DEFINED

#endif

// We also define our own interrupt tables in platform.S...
#define CYG_HAL_MIPS_ISR_TABLES_DEFINED

#------------------------------------------------------------------------------
# MEMC macros.
#

#if defined(CYG_HAL_STARTUP_ROM)

#------------------------------------------------------------------------------
# Make sure the jump to _start in vectors.S is done uncached
#

#define CYGARC_START_FUNC_UNCACHED

	.macro	hal_memc_init

	.extern	lowlevel_init
	.extern ath_ddr_initial_config

	jal	lowlevel_init
	nop

	#
	# At this point RedBoot is not ready to invoke 'C' functions.
	# However, we want to invoke a 'C' function for DDR init.
	# Hence provide a temporary stack location in SRAM
	#
	lui	sp, 0xbd00
	addiu   sp, sp, 0x1000

	lar	k0,ath_ddr_initial_config
	CYGARC_ADDRESS_REG_UNCACHED(k0)
	jalr	k0
	nop

	.endm

#define CYGPKG_HAL_MIPS_MEMC_DEFINED

#endif

#------------------------------------------------------------------------------
#endif // ifndef CYGONCE_HAL_PLATFORM_INC
# end of platform.inc
