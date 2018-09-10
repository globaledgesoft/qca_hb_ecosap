##
#=============================================================================
##	platform.S
##
##	QCA953X Malta platform code
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
## Author(s):	dmoseley
## Contributors:dmoseley, jskov
## Date:	2001-03-20
## Purpose:	MIPS Malta platform code
## Description:	Platform specific code for Malta board.
##
##
##
##
######DESCRIPTIONEND####
##
##=============================================================================

#include <pkgconf/system.h>
#include <pkgconf/hal.h>

#ifdef CYGPKG_KERNEL
# include <pkgconf/kernel.h>
#endif

#include <cyg/hal/arch.inc>
#include <cyg/hal/plf_io.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/plf_defs.inc>
#include CYGBLD_HAL_PLATFORM_H

#include <cyg/hal/qca.h>

##-----------------------------------------------------------------------------

##-----------------------------------------------------------------------------
# Platform Initialization.
# This code performs platform specific initialization.

/*
 * Helper macros.
 * These Clobber t7, t8 and t9
 */
#define reg_write(_reg, _val)			\
	li	t7,	(KSEG1ADDR(_reg));	\
	li	t8,	_val;			\
	sw	t8,	0(t7);

#define reg_rmw_set(_reg, _mask, _val)		\
	li	t7,	KSEG1ADDR(_reg);	\
	lw	t8,	0(t7);			\
	li	t9,	~(_mask);		\
	and	t8,	t8,	t9;		\
	li	t9,	_val;			\
	or	t8,	t8,	t9;		\
	sw	t8,	0(t7)

#define cpu_pll_set(_mask, _val)	\
	reg_rmw_set(CPU_PLL_CONFIG_ADDRESS, _mask, _val)

#define ddr_pll_set(_mask, _val)	\
	reg_rmw_set(DDR_PLL_CONFIG_ADDRESS, _mask, _val)

#define cpu_ddr_control_set(_mask, _val)	\
	reg_rmw_set(CPU_DDR_CLOCK_CONTROL_ADDRESS, _mask, _val)


/******************************************************************************
 * first level initialization:
 *
 * 0) If clock cntrl reset switch is already set, we're recovering from
 *    "divider reset"; goto 3.
 * 1) Setup divide ratios.
 * 2) Reset.
 * 3) Setup pll's, wait for lock.
 *
 *****************************************************************************/

.globl lowlevel_init
	.type	lowlevel_init, @function
	.text
	.align 4

lowlevel_init:
#if 0
#if !defined(CONFIG_ATH_EMULATION)

	reg_write(BB_DPLL2_ADDRESS, BB_DPLL2_KI_SET(2) | \
				BB_DPLL2_KD_SET(0x10) | \
				BB_DPLL2_PLL_PWD_SET(1));
	reg_write(PCIe_DPLL2_ADDRESS, PCIe_DPLL2_KI_SET(2) | \
				PCIe_DPLL2_KD_SET(0x10) | \
				PCIe_DPLL2_PLL_PWD_SET(1));
	reg_write(DDR_DPLL2_ADDRESS, DDR_DPLL2_KI_SET(2) | \
				DDR_DPLL2_KD_SET(0x10) | \
				DDR_DPLL2_PLL_PWD_SET(1));
	reg_write(CPU_DPLL2_ADDRESS, CPU_DPLL2_KI_SET(2) | \
				CPU_DPLL2_KD_SET(0x10) | \
				CPU_DPLL2_PLL_PWD_SET(1));

	li	t5,	CPU_PLL_CONFIG_NINT_VAL
	li	t6,	DDR_PLL_CONFIG_NINT_VAL
	li	t4,	CPU_PLL_DITHER_VAL
	li	t3,	DDR_PLL_DITHER_VAL

	li	t7,	PLL_CONFIG_VAL_F
	lw	t8,	0(t7)
	li	t7,	PLL_MAGIC
	beq	t7,	t8,	read_from_flash
	nop
	j	pll_bypass_set
	nop
read_from_flash:
	li	t7,	PLL_CONFIG_VAL_F + 4
	lw	t5,	0(t7)
	lw	t4,	4(t7)
	lw	t6,	8(t7)
	lw	t3,	12(t7)


pll_bypass_set:
	cpu_ddr_control_set (CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(1));
	cpu_ddr_control_set (CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_SET(1));
	cpu_ddr_control_set (CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_SET(1));

init_cpu_pll:
	li	t7,	KSEG1ADDR(CPU_PLL_CONFIG_ADDRESS);
	li	t8,	(CPU_PLL_CONFIG_PLLPWD_SET(1) | \
			CPU_PLL_CONFIG_REF_DIV_VAL | \
			CPU_PLL_CONFIG_RANGE_VAL | \
			CPU_PLL_CONFIG_OUT_DIV_VAL1);
	or	t8,	t8,	t5
	sw	t8,	0(t7);

init_ddr_pll:
	li	t7,	KSEG1ADDR(DDR_PLL_CONFIG_ADDRESS);
	li	t8,	(DDR_PLL_CONFIG_PLLPWD_SET(1) | \
			DDR_PLL_CONFIG_REF_DIV_VAL | \
			DDR_PLL_CONFIG_RANGE_VAL | \
			DDR_PLL_CONFIG_OUT_DIV_VAL1);
	or	t8,	t8,	t6
	sw	t8,	0(t7);

init_ahb_pll:
	reg_write(CPU_DDR_CLOCK_CONTROL_ADDRESS,
			CPU_DDR_CLOCK_CONTROL_AHB_DIV_VAL |
			AHB_CLK_FROM_DDR |
			CLK_SRC_CONTROL |
			CPU_DDR_CLOCK_CONTROL_DDR_POST_DIV |
			CPU_DDR_CLOCK_CONTROL_CPU_POST_DIV |
			CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(1) |
			CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_SET(1) |
			CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_SET(1));

pll_pwd_unset:
	cpu_pll_set(CPU_PLL_CONFIG_PLLPWD_MASK, CPU_PLL_CONFIG_PLLPWD_SET(0));
	ddr_pll_set(DDR_PLL_CONFIG_PLLPWD_MASK, DDR_PLL_CONFIG_PLLPWD_SET(0));

outdiv_unset:
	cpu_pll_set(CPU_PLL_CONFIG_OUTDIV_MASK, CPU_PLL_CONFIG_OUT_DIV_VAL2);
	ddr_pll_set(DDR_PLL_CONFIG_OUTDIV_MASK, DDR_PLL_CONFIG_OUT_DIV_VAL2);

pll_bypass_unset:
	cpu_ddr_control_set(CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_CPU_PLL_BYPASS_SET(0));
	cpu_ddr_control_set(CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_DDR_PLL_BYPASS_SET(0));
	cpu_ddr_control_set(CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_MASK, CPU_DDR_CLOCK_CONTROL_AHB_PLL_BYPASS_SET(0));

ddr_pll_dither_unset:
	li	t7,	KSEG1ADDR(DDR_PLL_DITHER_ADDRESS);
	sw	t3,	0(t7);

cpu_pll_dither_unset:
	li	t7,	KSEG1ADDR(CPU_PLL_DITHER_ADDRESS);
	sw	t4,	0(t7);

	lui	t7,	47110;
	lui	t8,	1020;
	sw	t8,	0xb4(t7);


#endif /* !defined(CONFIG_ATH_EMULATION) */
#endif
	jr ra
	nop

##-----------------------------------------------------------------------------
# Interrupt vector tables.
# These tables contain the isr, data and object pointers used to deliver
# interrupts to user code.

	.extern hal_default_isr

	.data

	.globl	hal_interrupt_handlers
hal_interrupt_handlers:
	.rept	CYGNUM_HAL_ISR_COUNT
	.long	hal_default_isr
	.endr

	.globl	hal_interrupt_data
hal_interrupt_data:
	.rept	CYGNUM_HAL_ISR_COUNT
	.long	0
	.endr

	.globl	hal_interrupt_objects
hal_interrupt_objects:
	.rept	CYGNUM_HAL_ISR_COUNT
	.long	0
	.endr

##-----------------------------------------------------------------------------
## end of platform.S
