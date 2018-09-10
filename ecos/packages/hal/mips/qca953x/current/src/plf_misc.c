//==========================================================================
//
//      plf_misc.c
//
//      HAL platform miscellaneous functions
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
// Contributors: nickg, jlarmour, dmoseley, jskov
// Date:         2001-03-20
// Purpose:      HAL miscellaneous functions
// Description:  This file contains miscellaneous functions provided by the
//               HAL.
//
//####DESCRIPTIONEND####
//
//========================================================================*/

#define CYGARC_HAL_COMMON_EXPORT_CPU_MACROS
#include <pkgconf/hal.h>

#include <cyg/infra/cyg_type.h>         // Base types
#include <cyg/infra/cyg_trac.h>         // tracing macros
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/infra/diag.h>

#include <cyg/hal/hal_arch.h>           // architectural definitions

#include <cyg/hal/hal_intr.h>           // Interrupt handling

#include <cyg/hal/hal_cache.h>          // Cache handling

#include <cyg/hal/hal_if.h>

#if defined(CYGPKG_IO_PCI)

#include <cyg/io/pci_hw.h>
#include <cyg/io/pci.h>

#endif // CYGPKG_IO_PCI

#include <cyg/hal/qca.h>
#include <cyg/hal/plf_intr.h>
#include <sys/types.h>

//--------------------------------------------------------------------------

#ifdef HAL_INIT_IRQ_WANTED
static void hal_init_irq(void);
#endif

//--------------------------------------------------------------------------
static cyg_handle_t misc_interrupt_handle;
static cyg_interrupt misc_interrupt_object;
typedef cyg_uint32 cyg_ISR(cyg_uint32 vector, CYG_ADDRWORD data, HAL_SavedRegisters *regs);  //this cyg_ISR is only used here
cyg_uint32 ath_dispatch_misc_intr(cyg_vector_t vector, cyg_addrword_t data, HAL_SavedRegisters *regs);

void hal_exception_init(void)
{
#ifdef HAL_VSR_SET_TO_ECOS_HANDLER
    // Reclaim the VSR off CygMon possibly
#ifdef CYGNUM_HAL_EXCEPTION_DATA_ACCESS
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_DATA_ACCESS, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_DATA_TLBMISS_ACCESS
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_DATA_TLBMISS_ACCESS, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_DATA_UNALIGNED_ACCESS
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_DATA_UNALIGNED_ACCESS, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_ILLEGAL_INSTRUCTION
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_ILLEGAL_INSTRUCTION, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_DIV_BY_ZERO
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_DIV_BY_ZERO, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_FPU
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_FPU, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_FPU_DIV_BY_ZERO
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_FPU_DIV_BY_ZERO, NULL );
#endif
#endif
}

void
hal_misc_intr_init(void)
{
    cyg_drv_interrupt_create(QCA_CPU_IRQ_MISC,
                             0,
                             0,
                             ath_dispatch_misc_intr,
                             NULL, &misc_interrupt_handle,
                             &misc_interrupt_object);

    cyg_drv_interrupt_attach(misc_interrupt_handle);
}

cyg_uint32
ath_dispatch_misc_intr(cyg_vector_t vector, cyg_addrword_t data, HAL_SavedRegisters *regs)
{
    int i;
    uint32_t mask, pending;
    cyg_ISR *isr;
    CYG_ADDRWORD isr_data;
    cyg_uint32 isr_ret = 0;
    externC volatile CYG_ADDRESS hal_interrupt_handlers[CYGNUM_HAL_ISR_COUNT];
    externC volatile CYG_ADDRWORD hal_interrupt_data[CYGNUM_HAL_ISR_COUNT];
    externC volatile CYG_ADDRESS hal_interrupt_objects[CYGNUM_HAL_ISR_COUNT];

    pending = ath_reg_rd(ATH_MISC_INT_STATUS) & ath_reg_rd(ATH_MISC_INT_MASK);

    for (mask = 1, i = QCA_MISC_IRQ_START; i <= QCA_MISC_IRQ_END;
         i++, mask <<= 1) {
        if (pending & mask) {
            isr = (cyg_ISR *) hal_interrupt_handlers[i];
            isr_data = hal_interrupt_data[i];
            isr_ret = (*isr) (vector, isr_data, regs);
            /* acknowledge the specific misc interrupt */
            ath_reg_rmw_clear(ATH_MISC_INT_STATUS, mask);
        }
    }

    /* acknowledge the MISC interrupt */
    cyg_interrupt_acknowledge(vector);
    return isr_ret;
}

#ifndef CYG_HAL_STARTUP_RAM
cyg_uint32
hal_ctrlc_isr(CYG_ADDRWORD vector, CYG_ADDRWORD data)
{
	return 0;
}
#endif

/*------------------------------------------------------------------------*/
/* Reset support                                                          */

void
hal_qca953x_reset(void)
{
    ath_reg_wr(RST_RESET_ADDRESS, ~0);

    for (;;);                            // wait for it
}

//--------------------------------------------------------------------------

#ifdef HAL_INIT_IRQ_WANTED
// IRQ init
static void
hal_init_irq(void)
{
}
#endif

/*------------------------------------------------------------------------*/
/* PCI support                                                            */
#if defined(CYGPKG_IO_PCI)

#define udelay			hal_delay_us

static int
__check_bar(cyg_uint32 addr, cyg_uint32 size)
{
    int n;

    for (n = 0; n <= 31; n++)
	if (size == (1 << n)) {
	    /* Check that address is naturally aligned */
	    if (addr != (addr & ~(size-1)))
		return 0;
	    return size - 1;
	}
    return 0;
}

static int
ath_local_read_config(int where, int size, cyg_uint32 *value)
{
	*value = ath_reg_rd(ATH_PCI_CRP + where);
	return 0;
}

static int
ath_local_write_config(int where, int size, cyg_uint32 value)
{
	ath_reg_wr((ATH_PCI_CRP + where),value);
	return 0;
}

#define QCA_MAX_PCI_BUS		0
#define QCA_MAX_PCI_DEVICE	0

#define qca_is_valid_pci_dev(b, d)	\
	(((b) == QCA_MAX_PCI_BUS) && ((d) == QCA_MAX_PCI_BUS))

cyg_uint32
cyg_hal_plf_pci_cfg_read_dword(cyg_uint32 bus,
                               cyg_uint32 devfn, cyg_uint32 offset)
{
	if (!(qca_is_valid_pci_dev(bus, devfn)))
		return 0xffffffff;

	return ath_reg_rd(ATH_PCI_DEV_CFGBASE + offset);
}

cyg_uint16
cyg_hal_plf_pci_cfg_read_word(cyg_uint32 bus,
                              cyg_uint32 devfn, cyg_uint32 offset)
{
	cyg_uint32 val;

	if (!(qca_is_valid_pci_dev(bus, devfn)))
		return 0xffff;

	val = cyg_hal_plf_pci_cfg_read_dword(bus, devfn, offset & ~3u);

	val = (val >> ((offset % 4) * 8));

	return val & 0xffffu;
}

cyg_uint8
cyg_hal_plf_pci_cfg_read_byte(cyg_uint32 bus,
                              cyg_uint32 devfn, cyg_uint32 offset)
{
	cyg_uint32 val;

	if (!(qca_is_valid_pci_dev(bus, devfn)))
		return 0xff;

	val = cyg_hal_plf_pci_cfg_read_dword(bus, devfn, offset & ~3u);

	val = (val >> ((offset % 4) * 8));

	return val & 0xffu;
}

void
cyg_hal_plf_pci_cfg_write_dword(cyg_uint32 bus,
				      cyg_uint32 devfn,
                                cyg_uint32 offset, cyg_uint32 data)
{
	if (!(qca_is_valid_pci_dev(bus, devfn)))
		return;

	ath_reg_wr((ATH_PCI_DEV_CFGBASE + offset), data);
}

void
cyg_hal_plf_pci_cfg_write_word(cyg_uint32 bus,
				     cyg_uint32 devfn,
                               cyg_uint32 offset, cyg_uint16 data)
{
	cyg_uint32 val;

	if (!(qca_is_valid_pci_dev(bus, devfn)))
		return;

	val = cyg_hal_plf_pci_cfg_read_dword(bus, devfn, offset & ~3u);

	val &= ~(0xffff << ((offset % 4) * 8));
	val |= (data << ((offset % 4) * 8));

	cyg_hal_plf_pci_cfg_write_dword(bus, devfn, offset & ~3u, val);
}

void
cyg_hal_plf_pci_cfg_write_byte(cyg_uint32 bus,
				     cyg_uint32 devfn,
                               cyg_uint32 offset, cyg_uint8 data)
{
	cyg_uint32 val;

	if (!(qca_is_valid_pci_dev(bus, devfn)))
		return;

	val = cyg_hal_plf_pci_cfg_read_dword(bus, devfn, offset & ~3u);

	val &= ~(0xff << ((offset % 4) * 8));
	val |= (data << ((offset % 4) * 8));

	cyg_hal_plf_pci_cfg_write_dword(bus, devfn, offset & ~3u, val);
}

void
qca_print_pci_info(void)
{
/*
 * If this is RedBoot, serial is not ready by now.
 * The prints don't work.
 */
#if 0
#ifndef CYG_HAL_STARTUP_ROM
	int i;
	cyg_uint8 devfn, req;
	cyg_pci_device_id devid;
	cyg_pci_device dev_info;

	devid = CYG_PCI_DEV_MAKE_ID(0, 0);
	devfn = CYG_PCI_DEV_GET_DEVFN(devid);

	cyg_pci_get_device_info(devid, &dev_info);

	HAL_PCI_CFG_READ_UINT8(0, 0, CYG_PCI_CFG_INT_PIN, req);

	diag_printf("\n");
	diag_printf("Bus: %d", CYG_PCI_DEV_GET_BUS(devid));
	diag_printf(", PCI Device: %d", CYG_PCI_DEV_GET_DEV(devfn));
	diag_printf(", PCI Func: %d\n", CYG_PCI_DEV_GET_FN(devfn));
	diag_printf("  Vendor Id: 0x%04X", dev_info.vendor);
	diag_printf(", Device Id: 0x%04X", dev_info.device);
	diag_printf(", Command: 0x%04X", dev_info.command);
	diag_printf(", IRQ: %d\n", req);
	for (i = 0; i < dev_info.num_bars; i++) {
		diag_printf("  BAR[%d]    0x%08x /", i, dev_info.base_address[i]);
		diag_printf(" probed size 0x%08x / CPU addr 0x%08x\n",
				dev_info.base_size[i], dev_info.base_map[i]);
	}
#endif
#endif
}

// One-time PCI initialization.

void
cyg_hal_plf_pci_init(void)
{
	cyg_uint8  next_bus = 1;
	cyg_uint32 cmd;

	if (is_drqfn() && !is_qca953x()) {
		/*
		 * Dont enable PCIe in DRQFN package as it has some issues
		 * related to PCIe
		 */
		return;
	}

	if (ath_reg_rd(RST_BOOTSTRAP_ADDRESS) & RST_BOOTSTRAP_TESTROM_ENABLE_MASK) {
		ath_reg_rmw_clear(RST_MISC2_ADDRESS, RST_MISC2_PERSTN_RCPHY_SET(1));

		ath_reg_wr(PCIE_PHY_REG_1_ADDRESS, PCIE_PHY_REG_1_RESET_1);
		ath_reg_wr(PCIE_PHY_REG_3_ADDRESS, PCIE_PHY_REG_3_RESET_1);

        ath_reg_rmw_set(PCIE_PWR_MGMT_ADDRESS,
                        PCIE_PWR_MGMT_ASSERT_CLKREQN_SET(1));

		ath_reg_rmw_set(PCIE_PLL_CONFIG_ADDRESS, PCIE_PLL_CONFIG_PLLPWD_SET(1));

		ath_reg_rmw_set(RST_RESET_ADDRESS, RST_RESET_PCIE_RESET_SET(1));
		ath_reg_rmw_set(RST_RESET_ADDRESS, RST_RESET_PCIE_PHY_RESET_SET(1));

		ath_reg_rmw_clear(RST_CLKGAT_EN_ADDRESS, RST_CLKGAT_EN_PCIE_RC_SET(1));

		return;
	} else {
		 /* Honeybee -The PCIe reference clock frequency is being changed
		    to vary from 99.968MHz to 99.999MHz using SS modulation */
		ath_reg_wr_nf(PCIE_PLL_DITHER_DIV_MAX_ADDRESS,
			PCIE_PLL_DITHER_DIV_MAX_EN_DITHER_SET(0x1) |
			PCIE_PLL_DITHER_DIV_MAX_USE_MAX_SET(0x1) |
			PCIE_PLL_DITHER_DIV_MAX_DIV_MAX_INT_SET(0x17) |
			PCIE_PLL_DITHER_DIV_MAX_DIV_MAX_FRAC_SET(0x3fff));

		ath_reg_wr_nf(PCIE_PLL_DITHER_DIV_MIN_ADDRESS,
			PCIE_PLL_DITHER_DIV_MIN_DIV_MIN_FRAC_SET(0x3f84)|
			PCIE_PLL_DITHER_DIV_MIN_DIV_MIN_INT_SET(0x17));
	}

	ath_reg_wr_nf(PCIE_PLL_CONFIG_ADDRESS,
		PCIE_PLL_CONFIG_REFDIV_SET(1) |
		PCIE_PLL_CONFIG_BYPASS_SET(1) |
		PCIE_PLL_CONFIG_PLLPWD_SET(1));
	udelay(10000);

	ath_reg_rmw_clear(PCIE_PLL_CONFIG_ADDRESS, PCIE_PLL_CONFIG_PLLPWD_SET(1));
	udelay(1000);
	ath_reg_rmw_clear(PCIE_PLL_CONFIG_ADDRESS, PCIE_PLL_CONFIG_BYPASS_SET(1));
	udelay(1000);

	ath_reg_rmw_set(RST_RESET_ADDRESS, RST_RESET_PCIE_PHY_RESET_SET(1));
	udelay(10000);

	ath_reg_rmw_set(RST_RESET_ADDRESS, RST_RESET_PCIE_RESET_SET(1));
	udelay(10000);

	ath_reg_wr_nf(PCIE_RESET_ADDRESS, 0);	// Put endpoint in reset
	udelay(100000);

	ath_reg_rmw_clear(RST_RESET_ADDRESS, RST_RESET_PCIE_PHY_RESET_SET(1));
	udelay(10000);

	ath_reg_rmw_clear(RST_RESET_ADDRESS, RST_RESET_PCIE_RESET_SET(1));
	udelay(10000);

	ath_reg_wr_nf(PCIE_APP_ADDRESS, PCIE_APP_PCIE_BAR_MSN_SET(1) |
					PCIE_APP_CFG_BE_SET(0xf) |
					PCIE_APP_SLV_RESP_ERR_MAP_SET(0x3f) |
					PCIE_APP_LTSSM_ENABLE_SET(1));

	cmd =	CYG_PCI_CFG_COMMAND_MEMORY |	CYG_PCI_CFG_COMMAND_MASTER |
		CYG_PCI_CFG_COMMAND_PARITY |	CYG_PCI_CFG_COMMAND_INVALIDATE |
		CYG_PCI_CFG_COMMAND_SERR |	CYG_PCI_CFG_COMMAND_FAST_BACK;

	ath_local_write_config(CYG_PCI_CFG_COMMAND, 4, cmd);
	ath_local_write_config(0x20, 4, 0x1ff01000);
	ath_local_write_config(0x24, 4, 0x1ff01000);

	ath_reg_wr_nf(PCIE_RESET_ADDRESS, 4);	// Pull endpoint out of reset
	udelay(100000);

	/*
	 * Check if the WLAN PCI-E H/W is present, If the
	 * WLAN H/W is not present, skip the PCI platform
	 * initialization code and return
	 */
	if (((ath_reg_rd(PCIE_RESET_ADDRESS)) & 0x1) == 0x0) {
#ifndef CYG_HAL_STARTUP_ROM
		/*
		 * If this is RedBoot, serial is not ready by now.
		 * The prints don't work.
		 */

//		diag_printf("*** Warning *** : PCIe WLAN Module not found !!!\n");
#endif
	}

	cyg_pci_init();

	if (cyg_pci_configure_bus(0, &next_bus))
		qca_print_pci_info();

	return;
}
#endif  // defined(CYGPKG_IO_PCI)

void
hal_platform_init(void)
{
    // Set up eCos/ROM interfaces
    hal_if_init();
#if 0 // defined(CYGPKG_IO_PCI)
    cyg_hal_plf_pci_init();
#endif
    hal_misc_intr_init();
    hal_exception_init();
}

/*------------------------------------------------------------------------*/
/* End of plf_misc.c                                                      */
