#ifndef CYGONCE_PLF_IO_H
#define CYGONCE_PLF_IO_H

//=============================================================================
//
//      plf_io.h
//
//      Platform specific IO support
//
//=============================================================================
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
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    dmoseley
// Contributors: dmoseley, jskov
// Date:         2001-03-20
// Purpose:      Malta platform IO support
// Description:
// Usage:        #include <cyg/hal/plf_io.h>
//
//####DESCRIPTIONEND####
//
//=============================================================================

#include <pkgconf/hal.h>
#include <cyg/hal/hal_misc.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/plf_intr.h>

#ifndef __ASSEMBLER__

externC void cyg_hal_plf_pci_init(void);
#define HAL_PCI_INIT()		do { ; } while (0)

#define HAL_PCI_ALLOC_BASE_MEMORY	0x84000000
#define HAL_PCI_PHYSICAL_MEMORY_BASE	CYGARC_UNCACHED_ADDRESS(0x08000000)

#define HAL_PCI_ALLOC_BASE_IO		0xdeadbeef
#define HAL_PCI_PHYSICAL_IO_BASE	0xdeadbeef

// Read a value from the PCI configuration space of the appropriate
// size at an address composed from the bus, devfn and offset.
#define HAL_PCI_CFG_READ_UINT8(__bus, __devfn, __offset, __val )	\
	__val = cyg_hal_plf_pci_cfg_read_byte((__bus),  (__devfn), (__offset));

#define HAL_PCI_CFG_READ_UINT16(__bus, __devfn, __offset, __val )	\
	__val = cyg_hal_plf_pci_cfg_read_word((__bus),  (__devfn), (__offset));

#define HAL_PCI_CFG_READ_UINT32(__bus, __devfn, __offset, __val )	\
	__val = cyg_hal_plf_pci_cfg_read_dword((__bus),  (__devfn), (__offset));

// Write a value to the PCI configuration space of the appropriate
// size at an address composed from the bus, devfn and offset.
#define HAL_PCI_CFG_WRITE_UINT8(__bus, __devfn, __offset, __val )	\
	cyg_hal_plf_pci_cfg_write_byte((__bus),  (__devfn), (__offset), (__val))

#define HAL_PCI_CFG_WRITE_UINT16(__bus, __devfn, __offset, __val )	\
	cyg_hal_plf_pci_cfg_write_word((__bus),  (__devfn), (__offset), (__val))

#define HAL_PCI_CFG_WRITE_UINT32(__bus, __devfn, __offset, __val )	\
	cyg_hal_plf_pci_cfg_write_dword((__bus),  (__devfn), (__offset), (__val))

#define HAL_PCI_TRANSLATE_INTERRUPT(__bus, __devfn, __vec, __valid)	\
	CYG_MACRO_START							\
		__valid = true;						\
	CYG_MACRO_END

#endif // __ASSEMBLER__
/* Nothing as of now, register definitions should go in here */
#endif // CYGONCE_PLF_IO_H
