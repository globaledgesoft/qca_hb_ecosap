#ifndef CYGONCE_DEVS_FLASH_SPI_QCA953X_H
#define CYGONCE_DEVS_FLASH_SPI_QCA953X_H

//=============================================================================
//
//      QCA953x.h
//
//      SPI flash driver for Silicon Storage Technology SST25xx devices
//      and compatibles.
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2008, 2009, 2011 Free Software Foundation, Inc.
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
// Author(s):   ccoutand, updated for Silicon Storage Technology SST25xx
//              flash
// Original(s): Chris Holgate
// Purpose:     Silicon Storage Technology SST25xxx SPI flash driver
//              implementation
//
//####DESCRIPTIONEND####
//
//=============================================================================

// Required data structures.
#include <cyg/io/flash_dev.h>

#define CONFIG_ATHEROS      1
#define CONFIG_MACH_QCA953x 1
#define CFG_INIT_STACK_IN_SRAM  1
#define CONFIG_BOARD953X    1
#define __CONFIG_BOARD_NAME board953x
#define CONFIG_BOARD_NAME "board953x"
#define BUILD_VERSION "9.5.5.36"
#define CFG_PLL_FREQ        CFG_PLL_550_400_200
#define CFG_ATHRS27_PHY 1
#define CFG_ATH_GMAC_NMACS 2
#define KUSEG           0x00000000
#define KSEG0           0x80000000
#define KSEG1           0xa0000000
#define KSEG2           0xc0000000
#define KSEG3           0xe0000000

#define KSEG0ADDR(a)        (((a) & 0x1fffffff) | KSEG0)
#define KSEG1ADDR(a)        (((a) & 0x1fffffff) | KSEG1)
#define KSEG2ADDR(a)        (((a) & 0x1fffffff) | KSEG2)
#define KSEG3ADDR(a)        (((a) & 0x1fffffff) | KSEG3)
#define FLASH_M25P64    0x00F2
#define FLASH_UNKNOWN   0xFFFF

#define ATHR_FLASH_START 0x9f000000
#define ATHR_FLASH_SIZE 0x00100000
#define ATHR_FLASH_SECTOR 0x1000

#define printf(...)
#include "board953x.h"
#define display(_x)
typedef unsigned char unchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef __signed char s8;
typedef unsigned char u8;
typedef __signed short s16;
typedef unsigned short u16;
typedef __signed int s32;
typedef unsigned int u32;

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
typedef unsigned char uchar;
typedef volatile unsigned long vu_long;
typedef volatile unsigned short vu_short;
typedef volatile unsigned char vu_char;

#define int8_t      char
#define int16_t     short
#define int32_t     long
#define int64_t     long long
#define uint8_t     u_char
#define uint16_t    u_short
#define uint32_t    u_long
#define uint64_t    unsigned long long
#define t_scalar_t  int
#define t_uscalar_t unsigned int
#define uintptr_t   unsigned long
#define ERR_OK              0
#define ERR_TIMOUT          1
#define ERR_NOT_ERASED          2
#define ERR_PROTECTED           4
#define ERR_INVAL           8
#define ERR_ALIGN           16
#define ERR_UNKNOWN_FLASH_VENDOR    32
#define ERR_UNKNOWN_FLASH_TYPE      64
#define ERR_PROG_ERROR          128

/*                                                                               
 * primitives                                                                    
 */

#define ath_be_msb(_val, _i) (((_val) & (1 << (7 - _i))) >> (7 - _i))

#define ath_spi_bit_banger(_byte)   do {               \
    int i;                             \
    for(i = 0; i < 8; i++) {                    \
        ath_reg_wr_nf(ATH_SPI_WRITE,                \
            ATH_SPI_CE_LOW | ath_be_msb(_byte, i));     \
        ath_reg_wr_nf(ATH_SPI_WRITE,                \
            ATH_SPI_CE_HIGH | ath_be_msb(_byte, i));    \
    }                               \
} while (0)

#define ath_spi_go()    do {                \
    ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CE_LOW);   \
    ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);   \
} while (0)

#define ath_spi_send_addr(__a) do {         \
    ath_spi_bit_banger(((__a & 0xff0000) >> 16));   \
    ath_spi_bit_banger(((__a & 0x00ff00) >> 8)); \
    ath_spi_bit_banger(__a & 0x0000ff);     \
} while (0)

#define ath_spi_delay_8()   ath_spi_bit_banger(0)
#define ath_spi_done()      ath_reg_wr_nf(ATH_SPI_FS, 0)

    typedef struct {
        ulong size;             /* total bank size in bytes     */
        ushort sector_count;    /* number of erase units        */
        ulong flash_id;         /* combined device & manufacturer code  */
        ulong start[CFG_MAX_FLASH_SECT];    /* physical sector start addresses */
        uchar protect[CFG_MAX_FLASH_SECT];  /* sector protection status    */
    } flash_info_t;
extern unsigned long flash_get_geom(flash_info_t * flash_info);

// Exported handle on the driver function table.
externC struct cyg_flash_dev_funs cyg_devs_flash_spi_QCA953x_funs;

//-----------------------------------------------------------------------------
// Macro used to generate a flash device object with the default QCA953X
// settings.  Even though the block info data structure is declared here, the
// details are not filled in until the device type is inferred during
// initialization.  This also applies to the 'end' field which is calculated
// using the _start_ address and the inferred size of the device.
// _name_   is the root name of the instantiated data structures.
// _start_  is the base address of the device - for SPI based devices this can
//          have an arbitrary value, since the device is not memory mapped.
// _spidev_ is a pointer to a SPI device object of type cyg_spi_device.  This
//          is not typechecked during compilation so be careful!

#define CYG_DEVS_FLASH_SPI_QCA953X_DRIVER(_name_, _start_, _spidev_)    \
struct cyg_flash_block_info _name_ ##_block_info;                       \
CYG_FLASH_DRIVER(_name_, &cyg_devs_flash_spi_QCA953x_funs, 0,           \
    _start_, _start_, 1, & _name_ ##_block_info, (void*) _spidev_)

//=============================================================================

//-----------------------------------------------------------------------------
#endif // CYGONCE_DEVS_FLASH_SPI_QCA953X_H
// EOF QCA953x.h
