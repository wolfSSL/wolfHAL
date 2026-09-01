/* pic32cz_uart.c
 *
 * Copyright (C) 2026 wolfSSL Inc.
 *
 * This file is part of wolfHAL.
 *
 * wolfHAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfHAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include <stdint.h>
#ifdef WHAL_CFG_PIC32CZ_UART_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_Pic32cz_Uart_Dev singleton */
#endif
#include <wolfHAL/uart/pic32cz_uart.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/* SERCOM USART Register Offsets */
#define USART_CTRLA_REG     0x00
#define USART_CTRLB_REG     0x04
#define USART_CTRLC_REG     0x08
#define USART_BAUD_REG      0x0C
#define USART_INTENCLR_REG  0x14
#define USART_INTENSET_REG  0x16
#define USART_INTFLAG_REG   0x18
#define USART_STATUS_REG    0x1A
#define USART_SYNCBUSY_REG  0x1C
#define USART_DATA_REG      0x28

/* CTRLA Register Bit Definitions */
#define USART_CTRLA_SWRST_Pos       0
#define USART_CTRLA_SWRST_Msk       (1UL << USART_CTRLA_SWRST_Pos)

#define USART_CTRLA_ENABLE_Pos      1
#define USART_CTRLA_ENABLE_Msk      (1UL << USART_CTRLA_ENABLE_Pos)

#define USART_CTRLA_MODE_Pos        2
#define USART_CTRLA_MODE_Msk        (WHAL_BITMASK(3) << USART_CTRLA_MODE_Pos)
#define USART_CTRLA_MODE_USART_INT_CLK 0x1

#define USART_CTRLA_RUNSTDBY_Pos    7
#define USART_CTRLA_RUNSTDBY_Msk    (1UL << USART_CTRLA_RUNSTDBY_Pos)

#define USART_CTRLA_IBON_Pos        8
#define USART_CTRLA_IBON_Msk        (1UL << USART_CTRLA_IBON_Pos)

#define USART_CTRLA_TXINV_Pos       9
#define USART_CTRLA_TXINV_Msk       (1UL << USART_CTRLA_TXINV_Pos)

#define USART_CTRLA_RXINV_Pos       10
#define USART_CTRLA_RXINV_Msk       (1UL << USART_CTRLA_RXINV_Pos)

#define USART_CTRLA_SAMPR_Pos       13
#define USART_CTRLA_SAMPR_Msk       (WHAL_BITMASK(3) << USART_CTRLA_SAMPR_Pos)
#define USART_CTRLA_SAMPR_16X_ARITH 0x0
#define USART_CTRLA_SAMPR_16X_FRAC  0x1
#define USART_CTRLA_SAMPR_8X_ARITH  0x2
#define USART_CTRLA_SAMPR_8X_FRAC   0x3
#define USART_CTRLA_SAMPR_3X_ARITH  0x4

#define USART_CTRLA_TXPO_Pos        16
#define USART_CTRLA_TXPO_Msk        (WHAL_BITMASK(2) << USART_CTRLA_TXPO_Pos)

#define USART_CTRLA_RXPO_Pos        20
#define USART_CTRLA_RXPO_Msk        (WHAL_BITMASK(2) << USART_CTRLA_RXPO_Pos)

#define USART_CTRLA_SAMPA_Pos       22
#define USART_CTRLA_SAMPA_Msk       (WHAL_BITMASK(2) << USART_CTRLA_SAMPA_Pos)

#define USART_CTRLA_FORM_Pos        24
#define USART_CTRLA_FORM_Msk        (WHAL_BITMASK(4) << USART_CTRLA_FORM_Pos)
#define USART_CTRLA_FORM_USART  0x0
#define USART_CTRLA_FORM_PARITY 0x1

#define USART_CTRLA_CMODE_Pos       28
#define USART_CTRLA_CMODE_Msk       (1UL << USART_CTRLA_CMODE_Pos)

#define USART_CTRLA_CPOL_Pos        29
#define USART_CTRLA_CPOL_Msk        (1UL << USART_CTRLA_CPOL_Pos)

#define USART_CTRLA_DORD_Pos        30
#define USART_CTRLA_DORD_Msk        (1UL << USART_CTRLA_DORD_Pos)

/* CTRLB Register Bit Definitions */
#define USART_CTRLB_CHSIZE_Pos      0
#define USART_CTRLB_CHSIZE_Msk      (WHAL_BITMASK(3) << USART_CTRLB_CHSIZE_Pos)
#define USART_CTRLB_CHSIZE_8BIT 0x0
#define USART_CTRLB_CHSIZE_9BIT 0x1
#define USART_CTRLB_CHSIZE_5BIT 0x5
#define USART_CTRLB_CHSIZE_6BIT 0x6
#define USART_CTRLB_CHSIZE_7BIT 0x7

#define USART_CTRLB_SBMODE_Pos      6
#define USART_CTRLB_SBMODE_Msk      (1UL << USART_CTRLB_SBMODE_Pos)

#define USART_CTRLB_COLDEN_Pos      8
#define USART_CTRLB_COLDEN_Msk      (1UL << USART_CTRLB_COLDEN_Pos)

#define USART_CTRLB_SFDE_Pos        9
#define USART_CTRLB_SFDE_Msk        (1UL << USART_CTRLB_SFDE_Pos)

#define USART_CTRLB_ENC_Pos         10
#define USART_CTRLB_ENC_Msk         (1UL << USART_CTRLB_ENC_Pos)

#define USART_CTRLB_PMODE_Pos       13
#define USART_CTRLB_PMODE_Msk       (1UL << USART_CTRLB_PMODE_Pos)

#define USART_CTRLB_TXEN_Pos        16
#define USART_CTRLB_TXEN_Msk        (1UL << USART_CTRLB_TXEN_Pos)

#define USART_CTRLB_RXEN_Pos        17
#define USART_CTRLB_RXEN_Msk        (1UL << USART_CTRLB_RXEN_Pos)

/* INTFLAG Register Bit Definitions */
#define USART_INTFLAG_DRE_Pos       0
#define USART_INTFLAG_DRE_Msk       (1UL << USART_INTFLAG_DRE_Pos)

#define USART_INTFLAG_TXC_Pos       1
#define USART_INTFLAG_TXC_Msk       (1UL << USART_INTFLAG_TXC_Pos)

#define USART_INTFLAG_RXC_Pos       2
#define USART_INTFLAG_RXC_Msk       (1UL << USART_INTFLAG_RXC_Pos)

#define USART_INTFLAG_RXS_Pos       3
#define USART_INTFLAG_RXS_Msk       (1UL << USART_INTFLAG_RXS_Pos)

#define USART_INTFLAG_CTSIC_Pos     4
#define USART_INTFLAG_CTSIC_Msk     (1UL << USART_INTFLAG_CTSIC_Pos)

#define USART_INTFLAG_RXBRK_Pos     5
#define USART_INTFLAG_RXBRK_Msk     (1UL << USART_INTFLAG_RXBRK_Pos)

#define USART_INTFLAG_ERROR_Pos     7
#define USART_INTFLAG_ERROR_Msk     (1UL << USART_INTFLAG_ERROR_Pos)

/* STATUS Register Bit Definitions */
#define USART_STATUS_PERR_Pos       0
#define USART_STATUS_PERR_Msk       (1UL << USART_STATUS_PERR_Pos)

#define USART_STATUS_FERR_Pos       1
#define USART_STATUS_FERR_Msk       (1UL << USART_STATUS_FERR_Pos)

#define USART_STATUS_BUFOVF_Pos     2
#define USART_STATUS_BUFOVF_Msk     (1UL << USART_STATUS_BUFOVF_Pos)

#define USART_STATUS_CTS_Pos        3
#define USART_STATUS_CTS_Msk        (1UL << USART_STATUS_CTS_Pos)

#define USART_STATUS_ISF_Pos        4
#define USART_STATUS_ISF_Msk        (1UL << USART_STATUS_ISF_Pos)

#define USART_STATUS_COLL_Pos       5
#define USART_STATUS_COLL_Msk       (1UL << USART_STATUS_COLL_Pos)

#define USART_STATUS_TXE_Pos        6
#define USART_STATUS_TXE_Msk        (1UL << USART_STATUS_TXE_Pos)

/* SYNCBUSY Register Bit Definitions */
#define USART_SYNCBUSY_SWRST_Pos    0
#define USART_SYNCBUSY_SWRST_Msk    (1UL << USART_SYNCBUSY_SWRST_Pos)

#define USART_SYNCBUSY_ENABLE_Pos   1
#define USART_SYNCBUSY_ENABLE_Msk   (1UL << USART_SYNCBUSY_ENABLE_Pos)

#define USART_SYNCBUSY_CTRLB_Pos    2
#define USART_SYNCBUSY_CTRLB_Msk    (1UL << USART_SYNCBUSY_CTRLB_Pos)

/* DATA Register */
#define USART_DATA_Pos              0
#define USART_DATA_Msk              (WHAL_BITMASK(9) << USART_DATA_Pos)

#ifdef WHAL_CFG_PIC32CZ_UART_DIRECT_API_MAPPING
#define whal_Pic32cz_Uart_Init      whal_Uart_Init
#define whal_Pic32cz_Uart_Deinit    whal_Uart_Deinit
#define whal_Pic32cz_Uart_Send      whal_Uart_Send
#define whal_Pic32cz_Uart_Recv      whal_Uart_Recv
#define whal_Pic32cz_Uart_SendAsync whal_Uart_SendAsync
#define whal_Pic32cz_Uart_RecvAsync whal_Uart_RecvAsync
#endif /* WHAL_CFG_PIC32CZ_UART_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_PIC32CZ_UART_SINGLE_INSTANCE
const whal_Uart whal_Pic32cz_Uart_Dev = WHAL_CFG_PIC32CZ_UART_DEV;
#endif

whal_Error whal_Pic32cz_Uart_Init(whal_Uart *uartDev)
{
    whal_Error err;
#ifdef WHAL_CFG_PIC32CZ_UART_SINGLE_INSTANCE
    whal_Pic32cz_Uart_Cfg *cfg =
        (whal_Pic32cz_Uart_Cfg *)whal_Pic32cz_Uart_Dev.cfg;
    size_t base = whal_Pic32cz_Uart_Dev.base;
    (void)uartDev;
#else
    whal_Pic32cz_Uart_Cfg *cfg;
    size_t base;

    if (!uartDev || !uartDev->cfg) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (whal_Pic32cz_Uart_Cfg *)uartDev->cfg;
#endif

    /* Configure CTRLA: internal clock, async mode, LSB first, 16x sampling */
    whal_Reg_Update(base, USART_CTRLA_REG,
                    USART_CTRLA_MODE_Msk |
                    USART_CTRLA_SAMPR_Msk |
                    USART_CTRLA_TXPO_Msk |
                    USART_CTRLA_RXPO_Msk |
                    USART_CTRLA_FORM_Msk |
                    USART_CTRLA_CMODE_Msk |
                    USART_CTRLA_DORD_Msk,
                    whal_SetBits(USART_CTRLA_MODE_Msk, USART_CTRLA_MODE_Pos, USART_CTRLA_MODE_USART_INT_CLK) |
                    whal_SetBits(USART_CTRLA_SAMPR_Msk, USART_CTRLA_SAMPR_Pos, USART_CTRLA_SAMPR_16X_ARITH) |
                    whal_SetBits(USART_CTRLA_TXPO_Msk, USART_CTRLA_TXPO_Pos, cfg->txPad) |
                    whal_SetBits(USART_CTRLA_RXPO_Msk, USART_CTRLA_RXPO_Pos, cfg->rxPad) |
                    whal_SetBits(USART_CTRLA_FORM_Msk, USART_CTRLA_FORM_Pos, USART_CTRLA_FORM_USART) |
                    whal_SetBits(USART_CTRLA_CMODE_Msk, USART_CTRLA_CMODE_Pos, 0) |  /* Async mode */
                    whal_SetBits(USART_CTRLA_DORD_Msk, USART_CTRLA_DORD_Pos, 1));   /* LSB first */

    /* Configure CTRLB: 8-bit char size, 1 stop bit, no parity */
    whal_Reg_Update(base, USART_CTRLB_REG,
                    USART_CTRLB_CHSIZE_Msk |
                    USART_CTRLB_SBMODE_Msk |
                    USART_CTRLB_PMODE_Msk,
                    whal_SetBits(USART_CTRLB_CHSIZE_Msk, USART_CTRLB_CHSIZE_Pos, USART_CTRLB_CHSIZE_8BIT) |
                    whal_SetBits(USART_CTRLB_SBMODE_Msk, USART_CTRLB_SBMODE_Pos, 0) |
                    whal_SetBits(USART_CTRLB_PMODE_Msk, USART_CTRLB_PMODE_Pos, 0));

    /* Wait for CTRLB sync */
    err = whal_Reg_ReadPoll(base, USART_SYNCBUSY_REG,
                                    USART_SYNCBUSY_CTRLB_Msk, 0, cfg->timeout);
    if (err != WHAL_SUCCESS)
        return err;

    /* Set baud rate */
    whal_Reg_Update(base, USART_BAUD_REG,
                    0xFFFF,
                    cfg->baud);

    /* Enable transmitter and receiver */
    whal_Reg_Update(base, USART_CTRLB_REG,
                    USART_CTRLB_TXEN_Msk | USART_CTRLB_RXEN_Msk,
                    whal_SetBits(USART_CTRLB_TXEN_Msk, USART_CTRLB_TXEN_Pos, 1) |
                    whal_SetBits(USART_CTRLB_RXEN_Msk, USART_CTRLB_RXEN_Pos, 1));

    /* Wait for CTRLB sync */
    err = whal_Reg_ReadPoll(base, USART_SYNCBUSY_REG,
                                    USART_SYNCBUSY_CTRLB_Msk, 0, cfg->timeout);
    if (err != WHAL_SUCCESS)
        return err;

    /* Enable SERCOM USART */
    whal_Reg_Update(base, USART_CTRLA_REG,
                    USART_CTRLA_ENABLE_Msk,
                    whal_SetBits(USART_CTRLA_ENABLE_Msk, USART_CTRLA_ENABLE_Pos, 1));

    /* Wait for enable sync */
    err = whal_Reg_ReadPoll(base, USART_SYNCBUSY_REG,
                                    USART_SYNCBUSY_ENABLE_Msk, 0, cfg->timeout);
    if (err != WHAL_SUCCESS)
        return err;

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Uart_Deinit(whal_Uart *uartDev)
{
    whal_Error err;
#ifdef WHAL_CFG_PIC32CZ_UART_SINGLE_INSTANCE
    whal_Pic32cz_Uart_Cfg *cfg =
        (whal_Pic32cz_Uart_Cfg *)whal_Pic32cz_Uart_Dev.cfg;
    size_t base = whal_Pic32cz_Uart_Dev.base;
    (void)uartDev;
#else
    size_t base;
    whal_Pic32cz_Uart_Cfg *cfg;

    if (!uartDev || !uartDev->cfg) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (whal_Pic32cz_Uart_Cfg *)uartDev->cfg;
#endif

    /* Disable SERCOM USART */
    whal_Reg_Update(base, USART_CTRLA_REG,
                    USART_CTRLA_ENABLE_Msk,
                    whal_SetBits(USART_CTRLA_ENABLE_Msk, USART_CTRLA_ENABLE_Pos, 0));

    /* Wait for disable sync */
    err = whal_Reg_ReadPoll(base, USART_SYNCBUSY_REG,
                                    USART_SYNCBUSY_ENABLE_Msk, 0, cfg->timeout);
    if (err != WHAL_SUCCESS)
        return err;

    /* Disable transmitter and receiver */
    whal_Reg_Update(base, USART_CTRLB_REG,
                    USART_CTRLB_TXEN_Msk | USART_CTRLB_RXEN_Msk,
                    whal_SetBits(USART_CTRLB_TXEN_Msk, USART_CTRLB_TXEN_Pos, 0) |
                    whal_SetBits(USART_CTRLB_RXEN_Msk, USART_CTRLB_RXEN_Pos, 0));

    /* Wait for CTRLB sync */
    err = whal_Reg_ReadPoll(base, USART_SYNCBUSY_REG,
                                    USART_SYNCBUSY_CTRLB_Msk, 0, cfg->timeout);
    if (err != WHAL_SUCCESS)
        return err;

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    const uint8_t *buf = data;
    whal_Error err;
#ifdef WHAL_CFG_PIC32CZ_UART_SINGLE_INSTANCE
    whal_Pic32cz_Uart_Cfg *cfg =
        (whal_Pic32cz_Uart_Cfg *)whal_Pic32cz_Uart_Dev.cfg;
    size_t base = whal_Pic32cz_Uart_Dev.base;
    (void)uartDev;

    if (!data) {
        return WHAL_EINVAL;
    }
#else
    size_t base;
    whal_Pic32cz_Uart_Cfg *cfg;

    if (!uartDev || !uartDev->cfg || !data) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (whal_Pic32cz_Uart_Cfg *)uartDev->cfg;
#endif

    for (size_t i = 0; i < dataSz; ++i) {
        /* Wait for data register to be empty */
        err = whal_Reg_ReadPoll(base, USART_INTFLAG_REG,
                                USART_INTFLAG_DRE_Msk, USART_INTFLAG_DRE_Msk,
                                cfg->timeout);
        if (err)
            return err;

        /* Write data to transmit register */
        whal_Reg_Write(base, USART_DATA_REG,
                        whal_SetBits(USART_DATA_Msk, USART_DATA_Pos, buf[i]));
    }

    /* Wait for transmission complete */
    err = whal_Reg_ReadPoll(base, USART_INTFLAG_REG,
                            USART_INTFLAG_TXC_Msk, USART_INTFLAG_TXC_Msk,
                            cfg->timeout);
    if (err)
        return err;

    /* Clear TXC flag by writing 1 (W1C: write only this bit, 0 elsewhere) */
    whal_Reg_Write(base, USART_INTFLAG_REG,
                   whal_SetBits(USART_INTFLAG_TXC_Msk, USART_INTFLAG_TXC_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz)
{
    uint8_t *buf = data;
#ifdef WHAL_CFG_PIC32CZ_UART_SINGLE_INSTANCE
    whal_Pic32cz_Uart_Cfg *cfg =
        (whal_Pic32cz_Uart_Cfg *)whal_Pic32cz_Uart_Dev.cfg;
    size_t base = whal_Pic32cz_Uart_Dev.base;
    (void)uartDev;

    if (!data) {
        return WHAL_EINVAL;
    }
#else
    size_t base;
    whal_Pic32cz_Uart_Cfg *cfg;

    if (!uartDev || !uartDev->cfg || !data) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (whal_Pic32cz_Uart_Cfg *)uartDev->cfg;
#endif

    for (size_t i = 0; i < dataSz; ++i) {
        size_t rxData;
        whal_Error err;

        /* Wait for receive complete */
        err = whal_Reg_ReadPoll(base, USART_INTFLAG_REG,
                                USART_INTFLAG_RXC_Msk, USART_INTFLAG_RXC_Msk,
                                cfg->timeout);
        if (err)
            return err;

        /* Read received data */
        whal_Reg_Get(base, USART_DATA_REG,
                     USART_DATA_Msk, USART_DATA_Pos, &rxData);

        buf[i] = (uint8_t)rxData;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Uart_SendAsync(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    (void)dataSz;
    if (!uartDev || !data)
        return WHAL_EINVAL;
    return WHAL_ENOTSUP;
}

whal_Error whal_Pic32cz_Uart_RecvAsync(whal_Uart *uartDev, void *data, size_t dataSz)
{
    (void)dataSz;
    if (!uartDev || !data)
        return WHAL_EINVAL;
    return WHAL_ENOTSUP;
}

#ifndef WHAL_CFG_PIC32CZ_UART_DIRECT_API_MAPPING
const whal_UartDriver whal_Pic32cz_Uart_Driver = {
    .Init = whal_Pic32cz_Uart_Init,
    .Deinit = whal_Pic32cz_Uart_Deinit,
    .Send = whal_Pic32cz_Uart_Send,
    .Recv = whal_Pic32cz_Uart_Recv,
    .SendAsync = whal_Pic32cz_Uart_SendAsync,
    .RecvAsync = whal_Pic32cz_Uart_RecvAsync,
};
#endif /* !WHAL_CFG_PIC32CZ_UART_DIRECT_API_MAPPING */
