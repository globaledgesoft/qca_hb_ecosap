 /*
  * Copyright (c) 2008, Atheros Communications Inc.
  *
  * Permission to use, copy, modify, and/or distribute this software for any
  * purpose with or without fee is hereby granted, provided that the above
  * copyright notice and this permission notice appear in all copies.
  *
  * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
  */

#ifndef _ATH_FLASH_H
#define _ATH_FLASH_H

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

#endif /* _ATH_FLASH_H */
