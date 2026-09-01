/* stm32f0_uart.c
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
#ifdef WHAL_CFG_STM32F0_UART_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_Stm32f0_Uart_Dev singleton (possibly via platform alias macro) */
#endif
#include <wolfHAL/uart/stm32f0_uart.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

#define UART_CR1_REG 0x00
#define UART_CR1_UE_Pos 0
#define UART_CR1_UE_Msk (1UL << UART_CR1_UE_Pos)
#define UART_CR1_RE_Pos 2
#define UART_CR1_RE_Msk (1UL << UART_CR1_RE_Pos)
#define UART_CR1_TE_Pos 3
#define UART_CR1_TE_Msk (1UL << UART_CR1_TE_Pos)

#define UART_BRR_REG 0x0C
#define UART_BRR_Pos 0
#define UART_BRR_Msk (WHAL_BITMASK(16) << UART_BRR_Pos)

#define UART_ISR_REG 0x1C
#define UART_ISR_RXNE_Pos 5
#define UART_ISR_RXNE_Msk (1UL << UART_ISR_RXNE_Pos)
#define UART_ISR_TC_Pos 6
#define UART_ISR_TC_Msk (1UL << UART_ISR_TC_Pos)
#define UART_ISR_TXE_Pos 7
#define UART_ISR_TXE_Msk (1UL << UART_ISR_TXE_Pos)

#define UART_RDR_REG 0x24
#define UART_RDR_Pos 0
#define UART_RDR_Msk (WHAL_BITMASK(9) << UART_RDR_Pos)

#define UART_TDR_REG 0x28
#define UART_TDR_Pos 0
#define UART_TDR_Msk (WHAL_BITMASK(9) << UART_TDR_Pos)

#ifdef WHAL_CFG_STM32F0_UART_DIRECT_API_MAPPING
#define whal_Stm32f0_Uart_Init      whal_Uart_Init
#define whal_Stm32f0_Uart_Deinit    whal_Uart_Deinit
#define whal_Stm32f0_Uart_Send      whal_Uart_Send
#define whal_Stm32f0_Uart_Recv      whal_Uart_Recv
#define whal_Stm32f0_Uart_SendAsync whal_Uart_SendAsync
#define whal_Stm32f0_Uart_RecvAsync whal_Uart_RecvAsync
#endif

#ifdef WHAL_CFG_STM32F0_UART_SINGLE_INSTANCE
const whal_Uart whal_Stm32f0_Uart_Dev = WHAL_CFG_STM32F0_UART_DEV;
#endif

whal_Error whal_Stm32f0_Uart_Init(whal_Uart *uartDev)
{
#ifdef WHAL_CFG_STM32F0_UART_SINGLE_INSTANCE
    const whal_Stm32f0_Uart_Cfg *cfg =
        (const whal_Stm32f0_Uart_Cfg *)whal_Stm32f0_Uart_Dev.cfg;
    size_t base = whal_Stm32f0_Uart_Dev.base;
    (void)uartDev;
#else
    whal_Stm32f0_Uart_Cfg *cfg;
    size_t base;

    if (!uartDev || !uartDev->cfg)
        return WHAL_EINVAL;

    base = uartDev->base;
    cfg = (whal_Stm32f0_Uart_Cfg *)uartDev->cfg;
#endif

    whal_Reg_Update(base, UART_BRR_REG, UART_BRR_Msk,
                    whal_SetBits(UART_BRR_Msk, UART_BRR_Pos, cfg->brr));

    /* Enable UE, RE, TE — no FIFOEN on STM32F0 */
    whal_Reg_Update(base, UART_CR1_REG,
                    UART_CR1_UE_Msk | UART_CR1_RE_Msk | UART_CR1_TE_Msk,
                    whal_SetBits(UART_CR1_UE_Msk, UART_CR1_UE_Pos, 1) |
                    whal_SetBits(UART_CR1_RE_Msk, UART_CR1_RE_Pos, 1) |
                    whal_SetBits(UART_CR1_TE_Msk, UART_CR1_TE_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f0_Uart_Deinit(whal_Uart *uartDev)
{
#ifdef WHAL_CFG_STM32F0_UART_SINGLE_INSTANCE
    size_t base = whal_Stm32f0_Uart_Dev.base;
    (void)uartDev;
#else
    size_t base;

    if (!uartDev)
        return WHAL_EINVAL;

    base = uartDev->base;
#endif

    whal_Reg_Update(base, UART_CR1_REG,
                    UART_CR1_UE_Msk | UART_CR1_RE_Msk | UART_CR1_TE_Msk,
                    whal_SetBits(UART_CR1_UE_Msk, UART_CR1_UE_Pos, 0) |
                    whal_SetBits(UART_CR1_RE_Msk, UART_CR1_RE_Pos, 0) |
                    whal_SetBits(UART_CR1_TE_Msk, UART_CR1_TE_Pos, 0));

    whal_Reg_Update(base, UART_BRR_REG, UART_BRR_Msk,
                    whal_SetBits(UART_BRR_Msk, UART_BRR_Pos, 0));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f0_Uart_Send(whal_Uart *uartDev, const void *data,
                                  size_t dataSz)
{
    const uint8_t *buf = data;
#ifdef WHAL_CFG_STM32F0_UART_SINGLE_INSTANCE
    const whal_Stm32f0_Uart_Cfg *cfg =
        (const whal_Stm32f0_Uart_Cfg *)whal_Stm32f0_Uart_Dev.cfg;
    size_t base = whal_Stm32f0_Uart_Dev.base;
    (void)uartDev;

    if (!data)
        return WHAL_EINVAL;
#else
    size_t base;
    whal_Stm32f0_Uart_Cfg *cfg;

    if (!uartDev || !uartDev->cfg || !data)
        return WHAL_EINVAL;

    base = uartDev->base;
    cfg = (whal_Stm32f0_Uart_Cfg *)uartDev->cfg;
#endif

    for (size_t i = 0; i < dataSz; ++i) {
        whal_Error err;
        whal_Reg_Update(base, UART_TDR_REG, UART_TDR_Msk,
                        whal_SetBits(UART_TDR_Msk, UART_TDR_Pos, buf[i]));

        err = whal_Reg_ReadPoll(base, UART_ISR_REG, UART_ISR_TC_Msk,
                                UART_ISR_TC_Msk, cfg->timeout);
        if (err)
            return err;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f0_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz)
{
    uint8_t *buf = data;
#ifdef WHAL_CFG_STM32F0_UART_SINGLE_INSTANCE
    const whal_Stm32f0_Uart_Cfg *cfg =
        (const whal_Stm32f0_Uart_Cfg *)whal_Stm32f0_Uart_Dev.cfg;
    size_t base = whal_Stm32f0_Uart_Dev.base;
    (void)uartDev;

    if (!data)
        return WHAL_EINVAL;
#else
    size_t base;
    whal_Stm32f0_Uart_Cfg *cfg;

    if (!uartDev || !uartDev->cfg || !data)
        return WHAL_EINVAL;

    base = uartDev->base;
    cfg = (whal_Stm32f0_Uart_Cfg *)uartDev->cfg;
#endif

    for (size_t i = 0; i < dataSz; ++i) {
        size_t d;
        whal_Error err = whal_Reg_ReadPoll(base, UART_ISR_REG,
                                           UART_ISR_RXNE_Msk,
                                           UART_ISR_RXNE_Msk, cfg->timeout);
        if (err)
            return err;

        whal_Reg_Get(base, UART_RDR_REG, UART_RDR_Msk, UART_RDR_Pos, &d);
        buf[i] = d;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f0_Uart_SendAsync(whal_Uart *uartDev, const void *data,
                                       size_t dataSz)
{
    (void)dataSz;
    if (!uartDev || !data)
        return WHAL_EINVAL;
    return WHAL_ENOTSUP;
}

whal_Error whal_Stm32f0_Uart_RecvAsync(whal_Uart *uartDev, void *data,
                                       size_t dataSz)
{
    (void)dataSz;
    if (!uartDev || !data)
        return WHAL_EINVAL;
    return WHAL_ENOTSUP;
}

#ifndef WHAL_CFG_STM32F0_UART_DIRECT_API_MAPPING
const whal_UartDriver whal_Stm32f0_Uart_Driver = {
    .Init = whal_Stm32f0_Uart_Init,
    .Deinit = whal_Stm32f0_Uart_Deinit,
    .Send = whal_Stm32f0_Uart_Send,
    .Recv = whal_Stm32f0_Uart_Recv,
    .SendAsync = whal_Stm32f0_Uart_SendAsync,
    .RecvAsync = whal_Stm32f0_Uart_RecvAsync,
};
#endif
