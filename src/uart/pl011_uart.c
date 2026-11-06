/* pl011_uart.c
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
#ifdef WHAL_CFG_PL011_UART_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides WHAL_CFG_PL011_UART_DEV initializer */
#endif
#include <wolfHAL/uart/pl011_uart.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

/* Register map: AM028 UART module. AMD names in the comments, Arm PL011
 * names in the macros. */

/* DATA: TX/RX FIFO. On read, bits 11:8 are the error flags of the byte. */
#define UART_DR_REG 0x00
#define UART_DR_DATA_Pos 0
#define UART_DR_DATA_Msk (WHAL_BITMASK(8) << UART_DR_DATA_Pos)

#define UART_DR_FE_Pos 8
#define UART_DR_FE_Msk (1UL << UART_DR_FE_Pos)

#define UART_DR_PE_Pos 9
#define UART_DR_PE_Msk (1UL << UART_DR_PE_Pos)

#define UART_DR_BE_Pos 10
#define UART_DR_BE_Msk (1UL << UART_DR_BE_Pos)

#define UART_DR_OE_Pos 11
#define UART_DR_OE_Msk (1UL << UART_DR_OE_Pos)

#define UART_DR_ERR_Msk (UART_DR_FE_Msk | UART_DR_PE_Msk | \
                         UART_DR_BE_Msk | UART_DR_OE_Msk)

/* ERR_STAT_CLR (RSR / ECR): any write clears the error flags. */
#define UART_ECR_REG 0x04

/* FLAG */
#define UART_FR_REG 0x18
#define UART_FR_BUSY_Pos 3
#define UART_FR_BUSY_Msk (1UL << UART_FR_BUSY_Pos)

#define UART_FR_RXFE_Pos 4
#define UART_FR_RXFE_Msk (1UL << UART_FR_RXFE_Pos)

#define UART_FR_TXFF_Pos 5
#define UART_FR_TXFF_Msk (1UL << UART_FR_TXFF_Pos)

#define UART_FR_TXFE_Pos 7
#define UART_FR_TXFE_Msk (1UL << UART_FR_TXFE_Pos)

/* BAUD_INTEGER: 16-bit DIVINT */
#define UART_IBRD_REG 0x24
#define UART_IBRD_Msk WHAL_BITMASK(16)

/* BAUD_FRACT: 6-bit DIVFRAC */
#define UART_FBRD_REG 0x28
#define UART_FBRD_Msk WHAL_BITMASK(6)

/* LINE_CTRL: bits 7:0 are used, the rest is reserved. */
#define UART_LCRH_REG 0x2C
#define UART_LCRH_Msk WHAL_BITMASK(8)

/* CTRL */
#define UART_CR_REG 0x30
#define UART_CR_UARTEN_Pos 0
#define UART_CR_UARTEN_Msk (1UL << UART_CR_UARTEN_Pos)

#define UART_CR_TXE_Pos 8
#define UART_CR_TXE_Msk (1UL << UART_CR_TXE_Pos)

#define UART_CR_RXE_Pos 9
#define UART_CR_RXE_Msk (1UL << UART_CR_RXE_Pos)

#ifdef WHAL_CFG_PL011_UART_DIRECT_API_MAPPING
#define whal_Pl011_Uart_Init      whal_Uart_Init
#define whal_Pl011_Uart_Deinit    whal_Uart_Deinit
#define whal_Pl011_Uart_Send      whal_Uart_Send
#define whal_Pl011_Uart_Recv      whal_Uart_Recv
#define whal_Pl011_Uart_SendAsync whal_Uart_SendAsync
#define whal_Pl011_Uart_RecvAsync whal_Uart_RecvAsync
#endif /* WHAL_CFG_PL011_UART_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_PL011_UART_SINGLE_INSTANCE
const whal_Uart whal_Pl011_Uart_Dev = WHAL_CFG_PL011_UART_DEV;
#endif

whal_Error whal_Pl011_Uart_Init(whal_Uart *uartDev)
{
    whal_Error err;
#ifdef WHAL_CFG_PL011_UART_SINGLE_INSTANCE
    const whal_Pl011_Uart_Cfg *cfg =
        (const whal_Pl011_Uart_Cfg *)whal_Pl011_Uart_Dev.cfg;
    uintptr_t base = whal_Pl011_Uart_Dev.base;
    (void)uartDev;
#else
    whal_Pl011_Uart_Cfg *cfg;
    uintptr_t base;

    if (!uartDev || !uartDev->cfg) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (whal_Pl011_Uart_Cfg *)uartDev->cfg;
#endif

    /* DIVINT is 16 bits and must not be 0; DIVFRAC is 6 bits
     * (AM026 ch. 63 p670). */
    if (cfg->ibrd == 0 || cfg->ibrd > UART_IBRD_Msk ||
        cfg->fbrd > UART_FBRD_Msk) {
        return WHAL_EINVAL;
    }

    /* Turn the UART off and let the last byte go out before changing the
     * setup (AM026 ch. 63 p669). */
    whal_Reg32_Write(base, UART_CR_REG, 0);
    err = whal_Reg32_ReadPoll(base, UART_FR_REG, UART_FR_BUSY_Msk, 0,
                              cfg->timeout);
    if (err) {
        return err;
    }

    whal_Reg32_Write(base, UART_IBRD_REG, cfg->ibrd);
    whal_Reg32_Write(base, UART_FBRD_REG, cfg->fbrd);
    /* IBRD and FBRD only take effect on a LCR_H write, so LCR_H goes
     * last (Arm PL011 TRM, UARTLCR_H). */
    whal_Reg32_Write(base, UART_LCRH_REG, cfg->lcrH & UART_LCRH_Msk);
    whal_Reg32_Write(base, UART_CR_REG,
                     UART_CR_UARTEN_Msk | UART_CR_TXE_Msk | UART_CR_RXE_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Pl011_Uart_Deinit(whal_Uart *uartDev)
{
    whal_Error err;
#ifdef WHAL_CFG_PL011_UART_SINGLE_INSTANCE
    const whal_Pl011_Uart_Cfg *cfg =
        (const whal_Pl011_Uart_Cfg *)whal_Pl011_Uart_Dev.cfg;
    uintptr_t base = whal_Pl011_Uart_Dev.base;
    (void)uartDev;
#else
    whal_Pl011_Uart_Cfg *cfg;
    uintptr_t base;

    if (!uartDev || !uartDev->cfg) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (whal_Pl011_Uart_Cfg *)uartDev->cfg;
#endif

    /* Turn the UART off and let the last byte go out before clearing the
     * divisors and the FIFOs (AM026 ch. 63 p669, Clear FIFO). */
    whal_Reg32_Write(base, UART_CR_REG, 0);
    err = whal_Reg32_ReadPoll(base, UART_FR_REG, UART_FR_BUSY_Msk, 0,
                              cfg->timeout);
    if (err) {
        return err;
    }

    whal_Reg32_Write(base, UART_IBRD_REG, 0);
    whal_Reg32_Write(base, UART_FBRD_REG, 0);
    /* The LCR_H write makes the cleared divisors take effect; FEN = 0
     * flushes the FIFOs. */
    whal_Reg32_Write(base, UART_LCRH_REG, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Pl011_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    const uint8_t *buf = data;
    whal_Error err;
#ifdef WHAL_CFG_PL011_UART_SINGLE_INSTANCE
    const whal_Pl011_Uart_Cfg *cfg =
        (const whal_Pl011_Uart_Cfg *)whal_Pl011_Uart_Dev.cfg;
    uintptr_t base = whal_Pl011_Uart_Dev.base;
    (void)uartDev;

    if (!data) {
        return WHAL_EINVAL;
    }
#else
    uintptr_t base;
    whal_Pl011_Uart_Cfg *cfg;

    if (!uartDev || !uartDev->cfg || !data) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (whal_Pl011_Uart_Cfg *)uartDev->cfg;
#endif

    for (size_t i = 0; i < dataSz; ++i) {
        /* Wait for room in the TX FIFO. */
        err = whal_Reg32_ReadPoll(base, UART_FR_REG, UART_FR_TXFF_Msk, 0,
                                  cfg->timeout);
        if (err) {
            return err;
        }
        whal_Reg32_Write(base, UART_DR_REG, buf[i]);
    }

    /* Wait until the FIFO is empty and the last stop bit is out
     * (AM026 ch. 63 p668, BUSY). */
    err = whal_Reg32_ReadPoll(base, UART_FR_REG, UART_FR_TXFE_Msk,
                              UART_FR_TXFE_Msk, cfg->timeout);
    if (err) {
        return err;
    }
    err = whal_Reg32_ReadPoll(base, UART_FR_REG, UART_FR_BUSY_Msk, 0,
                              cfg->timeout);
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Pl011_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz)
{
    uint8_t *buf = data;
    uint32_t dr;
    whal_Error err;
#ifdef WHAL_CFG_PL011_UART_SINGLE_INSTANCE
    const whal_Pl011_Uart_Cfg *cfg =
        (const whal_Pl011_Uart_Cfg *)whal_Pl011_Uart_Dev.cfg;
    uintptr_t base = whal_Pl011_Uart_Dev.base;
    (void)uartDev;

    if (!data) {
        return WHAL_EINVAL;
    }
#else
    uintptr_t base;
    whal_Pl011_Uart_Cfg *cfg;

    if (!uartDev || !uartDev->cfg || !data) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (whal_Pl011_Uart_Cfg *)uartDev->cfg;
#endif

    for (size_t i = 0; i < dataSz; ++i) {
        /* Wait for a byte in the RX FIFO. */
        err = whal_Reg32_ReadPoll(base, UART_FR_REG, UART_FR_RXFE_Msk, 0,
                                  cfg->timeout);
        if (err) {
            return err;
        }

        /* One read: it takes the byte and its error flags off the FIFO. */
        dr = whal_Reg32_Read(base, UART_DR_REG);
        if (dr & UART_DR_ERR_Msk) {
            /* Clear the error flags; the value written does not matter
             * (AM028 UART ERR_STAT_CLR). */
            whal_Reg32_Write(base, UART_ECR_REG, 0);
            return WHAL_EHARDWARE;
        }

        buf[i] = (uint8_t)(dr & UART_DR_DATA_Msk);
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Pl011_Uart_SendAsync(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    (void)dataSz;
    if (!uartDev || !data) {
        return WHAL_EINVAL;
    }
    return WHAL_ENOTSUP;
}

whal_Error whal_Pl011_Uart_RecvAsync(whal_Uart *uartDev, void *data, size_t dataSz)
{
    (void)dataSz;
    if (!uartDev || !data) {
        return WHAL_EINVAL;
    }
    return WHAL_ENOTSUP;
}

#ifndef WHAL_CFG_PL011_UART_DIRECT_API_MAPPING
const whal_UartDriver whal_Pl011_Uart_Driver = {
    .Init = whal_Pl011_Uart_Init,
    .Deinit = whal_Pl011_Uart_Deinit,
    .Send = whal_Pl011_Uart_Send,
    .Recv = whal_Pl011_Uart_Recv,
    .SendAsync = whal_Pl011_Uart_SendAsync,
    .RecvAsync = whal_Pl011_Uart_RecvAsync,
};
#endif /* !WHAL_CFG_PL011_UART_DIRECT_API_MAPPING */
