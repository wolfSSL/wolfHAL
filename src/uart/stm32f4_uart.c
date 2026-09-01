/* stm32f4_uart.c
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
#ifdef WHAL_CFG_STM32F4_UART_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_Stm32f4_Uart_Dev singleton (possibly via platform alias macro) */
#endif
#include <wolfHAL/uart/stm32f4_uart.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

/*
 * STM32F4 USART Register Definitions (USARTv1)
 *
 * The STM32F4 USART uses SR/DR style registers which differ from the
 * ISR/TDR/RDR layout on newer STM32 families.
 */

/* Status Register */
#define UART_SR_REG 0x00
#define UART_SR_RXNE_Pos 5                                             /* Receive data register not empty */
#define UART_SR_RXNE_Msk (1UL << UART_SR_RXNE_Pos)

#define UART_SR_TC_Pos 6                                               /* Transmission complete */
#define UART_SR_TC_Msk (1UL << UART_SR_TC_Pos)

#define UART_SR_TXE_Pos 7                                              /* Transmit data register empty */
#define UART_SR_TXE_Msk (1UL << UART_SR_TXE_Pos)

/* Data Register - shared TX/RX */
#define UART_DR_REG 0x04
#define UART_DR_Pos 0
#define UART_DR_Msk (WHAL_BITMASK(9) << UART_DR_Pos)

/* Baud Rate Register */
#define UART_BRR_REG 0x08
#define UART_BRR_Pos 0
#define UART_BRR_Msk (WHAL_BITMASK(16) << UART_BRR_Pos)

/* Control Register 1 */
#define UART_CR1_REG 0x0C
#define UART_CR1_RE_Pos 2                                              /* Receiver enable */
#define UART_CR1_RE_Msk (1UL << UART_CR1_RE_Pos)

#define UART_CR1_TE_Pos 3                                              /* Transmitter enable */
#define UART_CR1_TE_Msk (1UL << UART_CR1_TE_Pos)

#define UART_CR1_UE_Pos 13                                             /* USART enable */
#define UART_CR1_UE_Msk (1UL << UART_CR1_UE_Pos)

#ifdef WHAL_CFG_STM32F4_UART_DIRECT_API_MAPPING
#define whal_Stm32f4_Uart_Init      whal_Uart_Init
#define whal_Stm32f4_Uart_Deinit    whal_Uart_Deinit
#define whal_Stm32f4_Uart_Send      whal_Uart_Send
#define whal_Stm32f4_Uart_Recv      whal_Uart_Recv
#define whal_Stm32f4_Uart_SendAsync whal_Uart_SendAsync
#define whal_Stm32f4_Uart_RecvAsync whal_Uart_RecvAsync
#endif /* WHAL_CFG_STM32F4_UART_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_STM32F4_UART_SINGLE_INSTANCE
const whal_Uart whal_Stm32f4_Uart_Dev = WHAL_CFG_STM32F4_UART_DEV;
#endif

whal_Error whal_Stm32f4_Uart_Init(whal_Uart *uartDev)
{
    uint32_t brr;
#ifdef WHAL_CFG_STM32F4_UART_SINGLE_INSTANCE
    const whal_Stm32f4_Uart_Cfg *cfg =
        (const whal_Stm32f4_Uart_Cfg *)whal_Stm32f4_Uart_Dev.cfg;
    size_t base = whal_Stm32f4_Uart_Dev.base;
    (void)uartDev;
#else
    whal_Stm32f4_Uart_Cfg *cfg;
    size_t base;

    if (!uartDev || !uartDev->cfg)
        return WHAL_EINVAL;

    base = uartDev->base;
    cfg = (whal_Stm32f4_Uart_Cfg *)uartDev->cfg;
#endif

    brr = cfg->brr;

    /* Set baud rate */
    whal_Reg_Update(base, UART_BRR_REG,
                    UART_BRR_Msk,
                    whal_SetBits(UART_BRR_Msk, UART_BRR_Pos, brr));

    /* Enable USART, transmitter, and receiver */
    whal_Reg_Update(base, UART_CR1_REG,
                    UART_CR1_UE_Msk | UART_CR1_RE_Msk | UART_CR1_TE_Msk,
                    whal_SetBits(UART_CR1_UE_Msk, UART_CR1_UE_Pos, 1) |
                    whal_SetBits(UART_CR1_RE_Msk, UART_CR1_RE_Pos, 1) |
                    whal_SetBits(UART_CR1_TE_Msk, UART_CR1_TE_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Uart_Deinit(whal_Uart *uartDev)
{
#ifdef WHAL_CFG_STM32F4_UART_SINGLE_INSTANCE
    size_t base = whal_Stm32f4_Uart_Dev.base;
    (void)uartDev;
#else
    size_t base;

    if (!uartDev)
        return WHAL_EINVAL;

    base = uartDev->base;
#endif

    /* Disable USART, transmitter, and receiver */
    whal_Reg_Update(base, UART_CR1_REG,
                    UART_CR1_UE_Msk | UART_CR1_RE_Msk | UART_CR1_TE_Msk,
                    whal_SetBits(UART_CR1_UE_Msk, UART_CR1_UE_Pos, 0) |
                    whal_SetBits(UART_CR1_RE_Msk, UART_CR1_RE_Pos, 0) |
                    whal_SetBits(UART_CR1_TE_Msk, UART_CR1_TE_Pos, 0));

    /* Clear baud rate */
    whal_Reg_Update(base, UART_BRR_REG,
                    UART_BRR_Msk,
                    whal_SetBits(UART_BRR_Msk, UART_BRR_Pos, 0));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    const uint8_t *buf = data;
#ifdef WHAL_CFG_STM32F4_UART_SINGLE_INSTANCE
    const whal_Stm32f4_Uart_Cfg *cfg =
        (const whal_Stm32f4_Uart_Cfg *)whal_Stm32f4_Uart_Dev.cfg;
    size_t base = whal_Stm32f4_Uart_Dev.base;
    (void)uartDev;

    if (!data)
        return WHAL_EINVAL;
#else
    size_t base;
    whal_Stm32f4_Uart_Cfg *cfg;

    if (!uartDev || !uartDev->cfg || !data)
        return WHAL_EINVAL;

    base = uartDev->base;
    cfg = (whal_Stm32f4_Uart_Cfg *)uartDev->cfg;
#endif

    for (size_t i = 0; i < dataSz; ++i) {
        whal_Error err;

        /* Wait for transmit data register empty */
        err = whal_Reg_ReadPoll(base, UART_SR_REG, UART_SR_TXE_Msk,
                                UART_SR_TXE_Msk, cfg->timeout);
        if (err)
            return err;

        /* Write byte to data register (must not read DR — use pure write) */
        whal_Reg_Write(base, UART_DR_REG, buf[i]);
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz)
{
    uint8_t *buf = data;
    size_t d;
#ifdef WHAL_CFG_STM32F4_UART_SINGLE_INSTANCE
    const whal_Stm32f4_Uart_Cfg *cfg =
        (const whal_Stm32f4_Uart_Cfg *)whal_Stm32f4_Uart_Dev.cfg;
    size_t base = whal_Stm32f4_Uart_Dev.base;
    (void)uartDev;

    if (!data)
        return WHAL_EINVAL;
#else
    size_t base;
    whal_Stm32f4_Uart_Cfg *cfg;

    if (!uartDev || !uartDev->cfg || !data)
        return WHAL_EINVAL;

    base = uartDev->base;
    cfg = (whal_Stm32f4_Uart_Cfg *)uartDev->cfg;
#endif

    for (size_t i = 0; i < dataSz; ++i) {
        /* Wait for receive data register not empty */
        whal_Error err = whal_Reg_ReadPoll(base, UART_SR_REG,
                                           UART_SR_RXNE_Msk,
                                           UART_SR_RXNE_Msk, cfg->timeout);
        if (err)
            return err;

        /* Read received byte */
        whal_Reg_Get(base, UART_DR_REG,
                     UART_DR_Msk, UART_DR_Pos, &d);

        buf[i] = d;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Uart_SendAsync(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    (void)dataSz;
    if (!uartDev || !data)
        return WHAL_EINVAL;
    return WHAL_ENOTSUP;
}

whal_Error whal_Stm32f4_Uart_RecvAsync(whal_Uart *uartDev, void *data, size_t dataSz)
{
    (void)dataSz;
    if (!uartDev || !data)
        return WHAL_EINVAL;
    return WHAL_ENOTSUP;
}

#ifndef WHAL_CFG_STM32F4_UART_DIRECT_API_MAPPING
const whal_UartDriver whal_Stm32f4_Uart_Driver = {
    .Init = whal_Stm32f4_Uart_Init,
    .Deinit = whal_Stm32f4_Uart_Deinit,
    .Send = whal_Stm32f4_Uart_Send,
    .SendAsync = whal_Stm32f4_Uart_SendAsync,
    .RecvAsync = whal_Stm32f4_Uart_RecvAsync,
    .Recv = whal_Stm32f4_Uart_Recv,
};
#endif /* !WHAL_CFG_UART_API_MAPPING */
