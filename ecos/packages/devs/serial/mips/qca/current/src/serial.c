/*
 * Copyright (c) 2014 The Linux Foundation. All rights reserved.
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
 *
 */
#include <cyg/dbg_print/dbg_print.h>    /* kernel logs */
#include <pkgconf/io_serial.h>
#include <pkgconf/io.h>

#include <cyg/io/io.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_if.h>
#include <cyg/io/devtab.h>
#include <cyg/infra/diag.h>
#include <cyg/io/serial.h>
#include <cyg/hal/plf_intr.h>
#include <pkgconf/system.h>
#include <cyg/hal/addrspace.h>
#include <cyg/hal/config.h>
#include <cyg/hal/qca.h>

#ifdef CYG_HAL_STARTUP_ROM
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#endif

int qca_serial_msec_timeout;
void qca_serial_puts(const char *s);
#define GPIO_OE_ADDRESS GPIO_OE
#define GPIO_OUT_ADDRESS GPIO_OUT

#ifdef ATH_ENABLE_SERIAL_INTERRUPTS

static cyg_handle_t serial_interrupt_handle;
static cyg_interrupt serial_interrupt_object;

static cyg_flag_t serial_events;
static cyg_uint8 srx_buf[256]; /* has to be 256 for logic to work */
static cyg_uint8 srx_rd, srx_wr;
static cyg_uint8 serial_msr, serial_lsr;

#define SERIAL_EVENT_RXRDY  0x01
#define SERIAL_EVENT_TBMT   0x02
#define SERIAL_EVENT_MSC    0x04
#define SERIAL_EVENT_LSC    0x08

// Interrupt status register
#define SIO_IIR_IID_MSC     0x00
#define SIO_IIR_IID_NONE    0x01
#define SIO_IIR_IID_THRE    0x02
#define SIO_IIR_IID_RXDA    0x04
#define SIO_IIR_IID_RXS     0x06
#define SIO_IIR_IID_CTO     0x0c

#define SIO_IIR_IID_MASK    0x0e
#define SIO_IER_RXDA        0x01 /* Receive data available */
#define SIO_IER_TRMT        0x02 /* Transmit Register Empty */
#define SIO_IER_LSI         0x04 /* Receive Line Status */
#define SIO_IER_MSI         0x08 /* Modem Status Interrupt */

cyg_uint32
qca_serial_intr(cyg_vector_t vector, cyg_addrword_t data,
                HAL_SavedRegisters * regs)
{
    cyg_uint8 i;

    CYGARC_HAL_SAVE_GP();

    while (!((i = ath_uart_rd(OFS_INTR_ID)) & SIO_IIR_IID_NONE)) {
        i &= SIO_IIR_IID_MASK;
        switch (i) {
        case SIO_IIR_IID_RXS:
            serial_lsr |= ath_uart_rd(OFS_LINE_STATUS);
            cyg_flag_setbits(&serial_events, SERIAL_EVENT_LSC);
            break;
        case SIO_IIR_IID_CTO:
        case SIO_IIR_IID_RXDA:
            while (ath_uart_rd(OFS_LINE_STATUS) & 0x1) {
                srx_buf[srx_wr++] = ath_uart_rd(OFS_RCV_BUFFER);
            }

            if (srx_wr - srx_rd) {
                cyg_flag_setbits(&serial_events, SERIAL_EVENT_RXRDY);
            }
            break;
        case SIO_IIR_IID_THRE:
            cyg_flag_setbits(&serial_events, SERIAL_EVENT_TBMT);
            break;
        case SIO_IIR_IID_MSC:
            serial_msr |= ath_uart_rd(OFS_MODEM_STATUS);
            cyg_flag_setbits(&serial_events, SERIAL_EVENT_MSC);
            break;
        }
    }

    CYGARC_HAL_RESTORE_GP();

    return CYG_ISR_HANDLED;
}

#endif /* ATH_ENABLE_SERIAL_INTERRUPTS */

static bool
qca_serial_init(struct cyg_devtab_entry *tab)
{
//#if !defined(CONFIG_ATH_EMULATION)
    uint32_t div, val;

	div = ATH_UART_FREQ / (16 * CONFIG_BAUDRATE);
#if defined(CONFIG_SCO_SLAVE_CONNECTED)
    val = ath_reg_rd(GPIO_OE_ADDRESS) & (~0xcbf410u);
#elif defined(CONFIG_MACH_QCA956x)
    val = ath_reg_rd(GPIO_OE_ADDRESS) & 0xbbfdf6;
#else
    val = ath_reg_rd(GPIO_OE_ADDRESS) & (~0xcffc10u);
#endif

#if defined(CONFIG_ATH_SPI_NAND_CS_GPIO)
    val |= 1 << CONFIG_ATH_SPI_NAND_CS_GPIO;
#endif
#if defined(CONFIG_ATH_SPI_CS1_GPIO)
    val |= 1 << CONFIG_ATH_SPI_CS1_GPIO;
#endif
    ath_reg_wr(GPIO_OE_ADDRESS, val);

#ifdef CONFIG_MACH_QCA956x

#if defined(UART_RX20_TX22)

    val = (ath_reg_rd(GPIO_OE_ADDRESS) & (~0x400000));
    ath_reg_wr(GPIO_OE_ADDRESS, val);

    ath_reg_rmw_clear(GPIO_OUT_FUNCTION5_ADDRESS,
                      GPIO_OUT_FUNCTION5_ENABLE_GPIO_22_MASK);

    ath_reg_rmw_set(GPIO_OUT_FUNCTION5_ADDRESS,
                    GPIO_OUT_FUNCTION5_ENABLE_GPIO_22_SET(0x16));

    ath_reg_rmw_clear(GPIO_IN_ENABLE0_ADDRESS, GPIO_IN_ENABLE0_UART_SIN_MASK);

    ath_reg_rmw_set(GPIO_IN_ENABLE0_ADDRESS,
                    GPIO_IN_ENABLE0_UART_SIN_SET(0x14));
#elif defined(UART_RX18_TX22)
    val = (ath_reg_rd(GPIO_OE_ADDRESS) & (~0x400000)) | 0x40000;
    ath_reg_wr(GPIO_OE_ADDRESS, val);

    ath_reg_rmw_clear(GPIO_OUT_FUNCTION5_ADDRESS,
                      GPIO_OUT_FUNCTION5_ENABLE_GPIO_22_MASK);
    ath_reg_rmw_set(GPIO_OUT_FUNCTION5_ADDRESS,
                    GPIO_OUT_FUNCTION5_ENABLE_GPIO_22_SET(0x16));
    ath_reg_rmw_clear(GPIO_IN_ENABLE0_ADDRESS, GPIO_IN_ENABLE0_UART_SIN_MASK);

    ath_reg_rmw_set(GPIO_IN_ENABLE0_ADDRESS,
                    GPIO_IN_ENABLE0_UART_SIN_SET(0x12));

#elif defined(UART_RX18_TX20)
    val = (ath_reg_rd(GPIO_OE_ADDRESS) & (~0x100000)) | 0x40000;
    ath_reg_wr(GPIO_OE_ADDRESS, val);

    val = ath_reg_rd(GPIO_OUT_ADDRESS) | 0xeffff6;
    ath_reg_wr(GPIO_OUT_ADDRESS, val);

    ath_reg_rmw_clear(GPIO_OUT_FUNCTION5_ADDRESS,
                      GPIO_OUT_FUNCTION5_ENABLE_GPIO_20_MASK);
    ath_reg_rmw_set(GPIO_OUT_FUNCTION5_ADDRESS,
                    GPIO_OUT_FUNCTION5_ENABLE_GPIO_20_SET(0x16));
    ath_reg_rmw_clear(GPIO_IN_ENABLE0_ADDRESS, GPIO_IN_ENABLE0_UART_SIN_MASK);

    ath_reg_rmw_set(GPIO_IN_ENABLE0_ADDRESS,
                    GPIO_IN_ENABLE0_UART_SIN_SET(0x12));

#elif defined(UART_RX24_TX20)
    // Turn off LED before XLNA swap to GPO
    val = ath_reg_rd(GPIO_OUT_ADDRESS) | 0xaffff6;
    ath_reg_wr(GPIO_OUT_ADDRESS, val);
    //Switch GPI and GPO and XPA, XLNA
    ath_reg_wr(GPIO_FUNCTION_ADDRESS, 0x8000);

    val = (ath_reg_rd(GPIO_OE_ADDRESS) & (~0x100000)) | 0x1000000;
    ath_reg_wr(GPIO_OE_ADDRESS, val);

    ath_reg_rmw_set(GPIO_OUT_FUNCTION5_ADDRESS,
                    GPIO_OUT_FUNCTION5_ENABLE_GPIO_20_SET(0x16));
    ath_reg_rmw_clear(GPIO_IN_ENABLE0_ADDRESS,
                      GPIO_IN_ENABLE0_UART_SIN_SET(0xff));

    ath_reg_rmw_set(GPIO_IN_ENABLE0_ADDRESS,
                    GPIO_IN_ENABLE0_UART_SIN_SET(0x18));

#elif defined(TEST_BOARD_UART)
    //Switch GPI and GPO and XPA1, ANTC
    ath_reg_wr(GPIO_FUNCTION_ADDRESS, 0xc000);

    val = ath_reg_rd(GPIO_OE_ADDRESS) & (~0x2000);
    ath_reg_wr(GPIO_OE_ADDRESS, val);

    ath_reg_rmw_clear(GPIO_OUT_FUNCTION3_ADDRESS,
                      GPIO_OUT_FUNCTION3_ENABLE_GPIO_13_MASK);

    ath_reg_rmw_set(GPIO_OUT_FUNCTION3_ADDRESS,
                    GPIO_OUT_FUNCTION3_ENABLE_GPIO_13_SET(0x16));

    ath_reg_rmw_clear(GPIO_IN_ENABLE0_ADDRESS,
                      GPIO_IN_ENABLE0_UART_SIN_SET(0xff));

    ath_reg_rmw_set(GPIO_IN_ENABLE0_ADDRESS,
                    GPIO_IN_ENABLE0_UART_SIN_SET(0x17));

#else
    val = (ath_reg_rd(GPIO_OE_ADDRESS) & (~0x100000)) | 0x80000;
    ath_reg_wr(GPIO_OE_ADDRESS, val);

    ath_reg_rmw_set(GPIO_OUT_FUNCTION5_ADDRESS,
                    GPIO_OUT_FUNCTION5_ENABLE_GPIO_20_SET(0x16));
    ath_reg_rmw_clear(GPIO_IN_ENABLE0_ADDRESS,
                      GPIO_IN_ENABLE0_UART_SIN_SET(0xff));

    ath_reg_rmw_set(GPIO_IN_ENABLE0_ADDRESS,
                    GPIO_IN_ENABLE0_UART_SIN_SET(0x13));

#endif

    val = ath_reg_rd(GPIO_OUT_ADDRESS) | 0xaffff6;
    ath_reg_wr(GPIO_OUT_ADDRESS, val);

    val = ath_reg_rd(GPIO_SPARE_ADDRESS);
    ath_reg_wr(GPIO_SPARE_ADDRESS, (val | 0x8402));

#else
    ath_reg_rmw_set(GPIO_OUT_FUNCTION2_ADDRESS,
                    GPIO_OUT_FUNCTION2_ENABLE_GPIO_10_SET(0x16));

    ath_reg_rmw_clear(GPIO_IN_ENABLE0_ADDRESS,
                      GPIO_IN_ENABLE0_UART_SIN_SET(0xff));

    ath_reg_rmw_set(GPIO_IN_ENABLE0_ADDRESS, GPIO_IN_ENABLE0_UART_SIN_SET(0x9));

    val = ath_reg_rd(GPIO_OUT_ADDRESS) | 0xcffc10u;
    ath_reg_wr(GPIO_OUT_ADDRESS, val);

    val = ath_reg_rd(GPIO_SPARE_ADDRESS);
    ath_reg_wr(GPIO_SPARE_ADDRESS, (val | 0x8402));

    ath_reg_wr(GPIO_OUT_ADDRESS, 0x2f);
#endif
    /* make sure interrupts are disabled */
    ath_uart_wr(OFS_INTR_ENABLE, 0);

    /*
     * set DIAB bit
     */
    ath_uart_wr(OFS_LINE_CONTROL, 0x80);

    /* set divisor */
    ath_uart_wr(OFS_DIVISOR_LSB, (div & 0xff));
    ath_uart_wr(OFS_DIVISOR_MSB, ((div >> 8) & 0xff));

    /* clear DIAB bit */
    ath_uart_wr(OFS_LINE_CONTROL, 0x00);

    /* set data format */
    ath_uart_wr(OFS_DATA_FORMAT, 0x3);

#ifdef ATH_ENABLE_SERIAL_INTERRUPTS

    HAL_INTERRUPT_MASK(QCA_MISC_IRQ_UART);

    cyg_flag_init(&serial_events);

    cyg_drv_interrupt_create(QCA_MISC_IRQ_UART,
                             0,
                             0,
                             qca_serial_intr,
                             NULL, &serial_interrupt_handle,
                             &serial_interrupt_object);

    cyg_drv_interrupt_attach(serial_interrupt_handle);

    /* enable RX_INT */
    ath_uart_wr(OFS_INTR_ENABLE, SIO_IER_RXDA);

    HAL_INTERRUPT_UNMASK(QCA_MISC_IRQ_UART);
#endif


    return true;
}

int
qca_serial_tstc(void)
{
    return (ath_uart_rd(OFS_LINE_STATUS) & 0x1);
}

#ifdef ATH_ENABLE_SERIAL_INTERRUPTS

uint8_t
qca_serial_getc(serial_channel * priv)
{
    while (!(srx_wr - srx_rd)) {
        cyg_flag_wait(&serial_events, SERIAL_EVENT_RXRDY,
                      CYG_FLAG_WAITMODE_OR | CYG_FLAG_WAITMODE_CLR);
    }

    return srx_buf[srx_rd++];
}

#else

uint8_t
qca_serial_getc(serial_channel * priv)
{
    while (!qca_serial_tstc());

    return ath_uart_rd(OFS_RCV_BUFFER);
}

#endif /* ATH_ENABLE_SERIAL_INTERRUPTS */

bool
qca_serial_putc(serial_channel * priv, uint8_t byte)
{
    if (byte == '\n')
        qca_serial_putc(priv, '\r');

    while (((ath_uart_rd(OFS_LINE_STATUS)) & 0x20) == 0x0);

    ath_uart_wr(OFS_SEND_BUFFER, byte);

    return true;
}

void
qca_serial_puts(const char *s)
{
    while (*s) {
        qca_serial_putc(NULL, *s++);
    }
}

#ifdef CYGSEM_HAL_VIRTUAL_VECTOR_SUPPORT

static void
qca_serial_write(void *__ch_data, const cyg_uint8 * __buf, cyg_uint32 __len)
{
    CYGARC_HAL_SAVE_GP();

    while (__len-- > 0)
        qca_serial_putc(__ch_data, *__buf++);

    CYGARC_HAL_RESTORE_GP();
}

static void
qca_serial_read(void *__ch_data, cyg_uint8 * __buf, cyg_uint32 __len)
{
    CYGARC_HAL_SAVE_GP();

    while (__len-- > 0)
        *__buf++ = qca_serial_getc(__ch_data);

    CYGARC_HAL_RESTORE_GP();
}

cyg_bool
qca_serial_getc_timeout(void *__ch_data, cyg_uint8 * ch)
{
    int delay_count;
    cyg_bool res = false;
    CYGARC_HAL_SAVE_GP();

    delay_count = qca_serial_msec_timeout * 10; // delay in .1 ms steps

    while (delay_count--) {
        if (qca_serial_tstc()) {
            *ch = ath_uart_rd(OFS_RCV_BUFFER);
            res = true;
            break;
        }
        CYGACC_CALL_IF_DELAY_US(100);
    }

    CYGARC_HAL_RESTORE_GP();
    return res;
}

static int
qca_serial_control(void *__ch_data, __comm_control_cmd_t __func, ...)
{
    int ret = 0;
    CYGARC_HAL_SAVE_GP();

    switch (__func) {
    case __COMMCTL_SET_TIMEOUT:
        {
            va_list ap;

            va_start(ap, __func);

            ret = qca_serial_msec_timeout;
            qca_serial_msec_timeout = va_arg(ap, cyg_uint32);

            va_end(ap);
        }
        break;
    case __COMMCTL_SETBAUD:
        break;

    case __COMMCTL_GETBAUD:
        ret = CONFIG_BAUDRATE;
        break;
    default:
        break;
    }
    CYGARC_HAL_RESTORE_GP();
    return ret;
}
#endif

// Set up the device characteristics; baud rate, etc.
static Cyg_ErrNo
qca_serial_set_config(serial_channel * chan, cyg_uint32 key,
                      const void *xbuf, cyg_uint32 * len)
{
    switch (key) {
    case CYG_IO_SET_CONFIG_SERIAL_INFO:
    default:
        return ENOERR;
    }
}

#if 0
void
cyg_hal_plf_comms_init(void)
{
#ifdef CYGSEM_HAL_VIRTUAL_VECTOR_SUPPORT
    hal_virtual_comm_table_t *comm;
    int cur =
        CYGACC_CALL_IF_SET_CONSOLE_COMM
        (CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);

    // Set channel 0
    CYGACC_CALL_IF_SET_CONSOLE_COMM(0);
    comm = CYGACC_CALL_IF_CONSOLE_PROCS();
    CYGACC_COMM_IF_CH_DATA_SET(*comm, NULL);
    CYGACC_COMM_IF_WRITE_SET(*comm, qca_serial_write);
    CYGACC_COMM_IF_READ_SET(*comm, qca_serial_read);
    CYGACC_COMM_IF_PUTC_SET(*comm, qca_serial_putc);
    CYGACC_COMM_IF_GETC_SET(*comm, qca_serial_getc);
    CYGACC_COMM_IF_CONTROL_SET(*comm, qca_serial_control);
    CYGACC_COMM_IF_DBG_ISR_SET(*comm, NULL);
    CYGACC_COMM_IF_GETC_TIMEOUT_SET(*comm, qca_serial_getc_timeout);

    // Restore original console
    CYGACC_CALL_IF_SET_CONSOLE_COMM(cur);
#endif
}
#endif

static Cyg_ErrNo
qca_serial_lookup(struct cyg_devtab_entry **tab,
                  struct cyg_devtab_entry *sub_tab,
                  const char *name)
{
    return ENOERR;
}

static SERIAL_FUNS(qca_serial_funs,
                   qca_serial_putc,
                   qca_serial_getc,
                   qca_serial_set_config,
                   NULL,
                   NULL
);

static SERIAL_CHANNEL(qca_serial_channel0,
                      qca_serial_funs,
                      qca_serial_msec_timeout,
                      CYG_SERIAL_BAUD_RATE(115200),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );

DEVTAB_ENTRY(qca_serial_io0,
             CYGDAT_IO_SERIAL_MIPS_QCA_SERIAL_NAME,
             0,                 // Does not depend on a lower level interface
             &cyg_io_serial_devio,
             qca_serial_init,
             qca_serial_lookup,     // Serial driver may need initializing
             &qca_serial_channel0
    );
