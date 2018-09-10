#ifndef CYGONCE_HAL_PLF_INTR_H
#define CYGONCE_HAL_PLF_INTR_H

//==========================================================================
//
//  plf_intr.h
//
//  Honeybee Interrupt and clock support
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later
// version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with eCos; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// As a special exception, if other files instantiate templates or use
// macros or inline functions from this file, or you compile this file
// and link it with other works to produce a work based on this file,
// this file does not by itself cause the resulting work to be covered by
// the GNU General Public License. However the source code for this file
// must still be made available in accordance with section (3) of the GNU
// General Public License v2.
//
// This exception does not invalidate any other reasons why a work based
// on this file might be covered by the GNU General Public License.
// -------------------------------------------
// ####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    nickg
// Contributors: nickg, jskov,
//               gthomas, jlarmour, dmoseley
// Date:         2001-03-20
// Purpose:      Define Interrupt support
// Description:  The macros defined here provide the HAL APIs for handling
//               interrupts and the clock for the Malta board.
//
// Usage:
//              #include <cyg/hal/plf_intr.h>
//              ...
//
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/hal.h>

// First an assembly safe part

//--------------------------------------------------------------------------
// Interrupt vectors.

#ifndef CYGHWR_HAL_INTERRUPT_VECTORS_DEFINED

#define CYGHWR_HAL_INTERRUPT_VECTORS_DEFINED

#define QCA_CPU_IRQ_WLAN	 	0
#define QCA_CPU_IRQ_USB		 	1
#define QCA_CPU_IRQ_GE0		 	2
#define QCA_CPU_IRQ_GE1		 	3
#define QCA_CPU_IRQ_MISC	 	4
#define QCA_CPU_IRQ_TIMER	 	5

    /* Miscellaneous interrupts */
#define QCA_MISC_IRQ_TIMER	 	6
#define QCA_MISC_IRQ_ERROR	 	7
#define QCA_MISC_IRQ_GPIO	 	8
#define QCA_MISC_IRQ_UART	 	9
#define QCA_MISC_IRQ_WDOG		10
#define QCA_MISC_IRQ_PERFC		11
#define QCA_MISC_IRQ_RSVRD6		12
#define QCA_MISC_IRQ_RSVRD7		13
#define QCA_MISC_IRQ_TIMER2		14
#define QCA_MISC_IRQ_TIMER3		15
#define QCA_MISC_IRQ_TIMER4		16
#define QCA_MISC_IRQ_RSVRD11	17
#define QCA_MISC_IRQ_ETHSW		18
#define QCA_MISC_IRQ_RSVRD13		19
#define QCA_MISC_IRQ_RSVRD14		20
#define QCA_MISC_IRQ_RSVRD15		21
#define QCA_MISC_IRQ_DDRSFENTRY		22
#define QCA_MISC_IRQ_DDRSFEXIT		23
#define QCA_MISC_IRQ_DDRACTIV		24
#define QCA_MISC_IRQ_RSVRD19		25
#define QCA_MISC_IRQ_RSVRD20		26
#define QCA_MISC_IRQ_USBPLLLOCK		27

#define	QCA_MISC_IRQ_START	(QCA_MISC_IRQ_TIMER)
#define	QCA_MISC_IRQ_END	(QCA_MISC_IRQ_USBPLLLOCK)

// Min/Max ISR numbers
#define CYGNUM_HAL_ISR_MIN	0
#define CYGNUM_HAL_ISR_MAX	QCA_MISC_IRQ_USBPLLLOCK
#define CYGNUM_HAL_ISR_COUNT	(CYGNUM_HAL_ISR_MAX - CYGNUM_HAL_ISR_MIN + 1)

#define CYGNUM_HAL_INTERRUPT_RTC            QCA_CPU_IRQ_TIMER

#define ATH_APB_BASE                    0x18000000  /* 384M */
#define ATH_RESET_BASE                  ATH_APB_BASE+0x00060000

/*
 * Reset block
 */
#define ATH_GENERAL_TMR                 ATH_RESET_BASE+0
#define ATH_GENERAL_TMR_RELOAD          ATH_RESET_BASE+4
#define ATH_WATCHDOG_TMR_CONTROL        ATH_RESET_BASE+8
#define ATH_WATCHDOG_TMR                ATH_RESET_BASE+0xc
#define ATH_MISC_INT_STATUS             ATH_RESET_BASE+0x10
#define ATH_MISC_INT_MASK               ATH_RESET_BASE+0x14

#endif // CYGHWR_HAL_INTERRUPT_VECTORS_DEFINED

//--------------------------------------------------------------------------
#ifndef __ASSEMBLER__

#include <cyg/infra/cyg_type.h>
#include <cyg/hal/plf_io.h>

#define __read_32bit_c0_register(source, sel)				\
({ unsigned int __res;							\
	if (sel == 0)							\
		__asm__ __volatile__(					\
			"mfc0\t%0, " #source "\n\t"			\
			: "=r" (__res));				\
	else								\
		__asm__ __volatile__(					\
			".set\tmips32\n\t"				\
			"mfc0\t%0, " #source ", " #sel "\n\t"		\
			".set\tmips0\n\t"				\
			: "=r" (__res));				\
	__res;								\
})

#define __write_32bit_c0_register(register, sel, value)			\
do {									\
	if (sel == 0)							\
		__asm__ __volatile__(					\
			"mtc0\t%z0, " #register "\n\t"			\
			: : "Jr" ((unsigned int)(value)));		\
	else								\
		__asm__ __volatile__(					\
			".set\tmips32\n\t"				\
			"mtc0\t%z0, " #register ", " #sel "\n\t"	\
			".set\tmips0"					\
			: : "Jr" ((unsigned int)(value)));		\
} while (0)

#define read_c0_status()	__read_32bit_c0_register($12, 0)
#define write_c0_status(val)	__write_32bit_c0_register($12, 0, val)

#define read_c0_cause()		__read_32bit_c0_register($13, 0)
#define write_c0_cause(val)	__write_32bit_c0_register($13, 0, val)

#define ath_reg_rd(_phys)       (*(volatile cyg_uint32 *)CYGARC_UNCACHED_ADDRESS(_phys))

#define ath_reg_wr_nf(_phys, _val) \
        ((*(volatile cyg_uint32 *)CYGARC_UNCACHED_ADDRESS(_phys)) = (_val))

#define ath_reg_wr(_phys, _val) do {    \
        ath_reg_wr_nf(_phys, _val);     \
        ath_reg_rd(_phys);              \
} while(0)

#define ath_reg_rmw_set(_reg, _mask)    do {                    \
        ath_reg_wr((_reg), (ath_reg_rd((_reg)) | (_mask)));     \
        ath_reg_rd((_reg));                                     \
} while(0)

#define ath_reg_rmw_clear(_reg, _mask) do {                     \
        ath_reg_wr((_reg), (ath_reg_rd((_reg)) & ~(_mask)));    \
        ath_reg_rd((_reg));                                     \
} while(0)

#define athr_reg_rmw_set                     ath_reg_rmw_set
#define athr_reg_rmw_clear                   ath_reg_rmw_clear
#define athr_reg_wr_nf                       ath_reg_wr_nf
#define athr_reg_wr                          ath_reg_wr
#define athr_reg_rd                          ath_reg_rd

//--------------------------------------------------------------------------
// Interrupt controller access.

#ifndef CYGHWR_HAL_INTERRUPT_CONTROLLER_ACCESS_DEFINED

// Array which stores the configured priority levels for the configured
// interrupts.
externC volatile CYG_BYTE hal_interrupt_level[CYGNUM_HAL_ISR_COUNT];

/*
 * CP0 status[8:15] = Interrupt Mask
 * The first two bits are for s/w interrupts. The hardware
 * interrupts start at 8 + 2 = 10.
 */
#define CP0_STATUS_IM(v)	(10 + (v))

#define HAL_INTERRUPT_MASK( _vector_ )				\
CYG_MACRO_START							\
	if (_vector_ <= QCA_CPU_IRQ_TIMER) {			\
		unsigned status = read_c0_status();		\
		status &= ~(1 << CP0_STATUS_IM(_vector_));	\
		write_c0_status(status);			\
	} else if (_vector_ <= QCA_MISC_IRQ_END) {		\
		unsigned status = athr_reg_rd(ATH_MISC_INT_MASK); \
		status &= ~(1 << (_vector_ - QCA_MISC_IRQ_START)); \
		athr_reg_wr(ATH_MISC_INT_MASK,status); 	\
	}							\
CYG_MACRO_END

#define HAL_INTERRUPT_UNMASK( _vector_ )			\
CYG_MACRO_START							\
	if (_vector_ <= QCA_CPU_IRQ_TIMER) {			\
		unsigned status = read_c0_status();		\
		status |= (1 << CP0_STATUS_IM(_vector_));	\
		write_c0_status(status);			\
	} else if (_vector_ <= QCA_MISC_IRQ_END) {               \
		/* Handle misc irq un-mask */			\
		unsigned status = athr_reg_rd(ATH_MISC_INT_MASK); \
		status |= (1 << (_vector_ - QCA_MISC_IRQ_START)); \
		athr_reg_wr(ATH_MISC_INT_MASK,status); 				\
	}							\
CYG_MACRO_END

#define HAL_INTERRUPT_ACKNOWLEDGE( _vector_ )			\
CYG_MACRO_START							\
	/* Handle misc interrupts alone. */			\
	/* For others the drivers take care */			\
CYG_MACRO_END

#define HAL_INTERRUPT_CONFIGURE( _vector_, _level_, _up_ )	\
CYG_MACRO_START							\
CYG_MACRO_END

#define HAL_INTERRUPT_SET_LEVEL( _vector_, _level_ )

#define CYGHWR_HAL_INTERRUPT_CONTROLLER_ACCESS_DEFINED

#endif

//----------------------------------------------------------------------------
// Reset.
#ifndef CYGHWR_HAL_RESET_DEFINED
extern void hal_qca953x_reset(void);
#define CYGHWR_HAL_RESET_DEFINED
#define HAL_PLATFORM_RESET()		hal_qca953x_reset()

#define HAL_PLATFORM_RESET_ENTRY	0x9f000000

#endif // CYGHWR_HAL_RESET_DEFINED

#endif // __ASSEMBLER__

//--------------------------------------------------------------------------
#endif // ifndef CYGONCE_HAL_PLF_INTR_H
// End of plf_intr.h
