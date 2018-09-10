//=============================================================================
//
//      mips_qca953x_flash.c
//
//      Flash programming for devices on MIPS QCA953x board
//
//=============================================================================

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

#include <cyg/dbg_print/dbg_print.h>    /* kernel logs */
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_diag.h>

#include <cyg/io/flash.h>
#include <cyg/io/flash_dev.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>

#include "devs_flash_spi_qca953x.h"

#include <string.h>

void
ath_spi_read_id(void)
{
    u32 rd;

    ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
    ath_spi_bit_banger(ATH_SPI_CMD_RDID);
    ath_spi_delay_8();
    ath_spi_delay_8();
    ath_spi_delay_8();
    ath_spi_go();

    rd = ath_reg_rd(ATH_SPI_RD_STATUS);

    PLAT_INFO("Flash Manuf Id 0x%x, DeviceId0 0x%x, DeviceId1 0x%x\n",
           (rd >> 16) & 0xff, (rd >> 8) & 0xff, (rd >> 0) & 0xff);
}

void
ath_spi_sector_erase(uint32_t addr)
{
    ath_spi_write_enable();
    ath_spi_bit_banger(ATH_SPI_CMD_SECTOR_ERASE_4K);
    ath_spi_send_addr(addr);
    ath_spi_go();
    display(0x7d);
    ath_spi_poll();
}

void
ath_spi_poll()
{
    int rd;

    do {
        ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
        ath_spi_bit_banger(ATH_SPI_CMD_RD_STATUS);
        ath_spi_delay_8();
        rd = (ath_reg_rd(ATH_SPI_RD_STATUS) & 1);
    } while (rd);
}

void
ath_spi_write_page(uint32_t addr, uint8_t * data, int len)
{
    int i;
    uint8_t ch;

    display(0x77);
    ath_spi_write_enable();
    ath_spi_bit_banger(ATH_SPI_CMD_PAGE_PROG);
    ath_spi_send_addr(addr);

    for (i = 0; i < len; i++) {
        ch = *(data + i);
        ath_spi_bit_banger(ch);
    }

    ath_spi_go();
    display(0x66);
    ath_spi_poll();
    display(0x6d);
}

void
ath_spi_write_enable()
{
    ath_reg_wr_nf(ATH_SPI_FS, 1);
    ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
    ath_spi_bit_banger(ATH_SPI_CMD_WREN);
    ath_spi_go();
}

//=============================================================================
// Utility functions for address calculations.
//=============================================================================

//-----------------------------------------------------------------------------
// Strips out any device address offset to give address within device.

static cyg_bool
qca953x_to_local_addr(struct cyg_flash_dev *dev, cyg_flashaddr_t * addr)
{
    cyg_bool retval = false;

    // Range check address before modifying it.
    if ((*addr >= dev->start) && (*addr <= dev->end)) {
        *addr -= dev->start;
        retval = true;
    }
    return retval;
}

static inline cyg_uint32
qca953x_spi_rdid(struct cyg_flash_dev *dev)
{
    u32 rd;
    ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
    ath_spi_bit_banger(ATH_SPI_CMD_RDID);
    ath_spi_delay_8();
    ath_spi_delay_8();
    ath_spi_delay_8();
    ath_spi_go();

    rd = ath_reg_rd(ATH_SPI_RD_STATUS);

    return rd;
}

//-----------------------------------------------------------------------------
// Implement reads to the specified buffer.

static inline void
qca953x_spi_read(struct cyg_flash_dev *dev, cyg_flashaddr_t addr,
                 cyg_uint8 * rbuf, cyg_uint32 rbuf_len)
{

    while (rbuf_len-- > 0) {

        *((unsigned char *)rbuf) = *((unsigned char *)addr);

        addr++;

        rbuf++;
    }
}

//-----------------------------------------------------------------------------
// Initialize the SPI flash, reading back the flash parameters.


static int
qca953x_init(struct cyg_flash_dev *dev)
{

    ath_reg_wr_nf(ATH_SPI_CLOCK, 0x43);
    ath_reg_rmw_set(ATH_SPI_FS, 1);
    ath_spi_read_id();
    ath_reg_rmw_clear(ATH_SPI_FS, 1);
    dev->start = ATHR_FLASH_START;
    dev->end = ATHR_FLASH_START + ATHR_FLASH_SIZE;
    dev->num_block_infos = 1;
    dev->init = 1;
    return FLASH_ERR_OK;

}

//-----------------------------------------------------------------------------
// Erase a single sector of the flash.

static int
qca953x_erase_block(struct cyg_flash_dev *dev, cyg_flashaddr_t block_base)
{
    ath_spi_sector_erase(block_base);
    ath_spi_done();
    return 0;
}

//-----------------------------------------------------------------------------
// Program an arbitrary number of pages into flash and verify written data.

static int
qca953x_program(struct cyg_flash_dev *dev, cyg_flashaddr_t addr,
                const void *source, size_t len)
{
    int total = 0, len_this_lp, bytes_this_page;
    ulong dst;
    uchar *src;

    PLAT_DEBUG("write addr: %x\n", addr);
    addr = addr - CFG_FLASH_BASE;

    while (total < len) {
        src = source + total;
        dst = addr + total;
        bytes_this_page = ATH_SPI_PAGE_SIZE - (addr % ATH_SPI_PAGE_SIZE);
        len_this_lp =
            ((len - total) > bytes_this_page) ? bytes_this_page : (len - total);
        ath_spi_write_page(dst, src, len_this_lp);
        total += len_this_lp;
    }

    ath_spi_done();

    return 0;

}

//-----------------------------------------------------------------------------
// Read back an arbitrary amount of data from flash.

static int
qca953x_read(struct cyg_flash_dev *dev, const cyg_flashaddr_t base,
             void *data, size_t len)
{
/* dummy function */
return 0;
}

//-----------------------------------------------------------------------------
// Lock device

static int
qca953x_lock(struct cyg_flash_dev *dev, cyg_flashaddr_t base)
{
/* dummy function */
return 0;
}

//-----------------------------------------------------------------------------
// Unlock device

static int
qca953x_unlock(struct cyg_flash_dev *dev, cyg_flashaddr_t base)
{
/* dummy function */
return 0;
}

//=============================================================================
// Fill in the driver data structures.
//=============================================================================
CYG_FLASH_FUNS(cyg_devs_flash_spi_qca953x_funs, // Exported name of function pointers.
               qca953x_init,    // Flash initialization.
               cyg_flash_devfn_query_nop,   // Query operations not supported.
               qca953x_erase_block, // Sector erase.
               qca953x_program, // Program multiple pages.
               NULL,            // Flash support normal memory reads .
               qca953x_lock,    // Locking (lock the whole device).
               qca953x_unlock);

static const cyg_flash_block_info_t cyg_flash_qca953x_block_info[1] = {
    {ATHR_FLASH_SECTOR, ATHR_FLASH_SIZE / ATHR_FLASH_SECTOR}
};

CYG_FLASH_DRIVER(cyg_flash_qca953x_flashdev, &cyg_devs_flash_spi_qca953x_funs, 0,   // Flags
                 ATHR_FLASH_START,    // Start
                 ATHR_FLASH_START + ATHR_FLASH_SIZE - 1,    // End
                 1,             // Number of block infos
                 cyg_flash_qca953x_block_info, NULL // priv
    );

//-----------------------------------------------------------------------------
// EOF qca953x.c
