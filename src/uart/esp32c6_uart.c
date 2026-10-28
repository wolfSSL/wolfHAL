/* esp32c6_uart.c
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

#include <wolfHAL/error.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/uart/esp32c6_uart.h>

#ifdef WHAL_CFG_ESP32C6_UART_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides WHAL_CFG_ESP32C6_UART_DEV initializer */
#endif

#ifdef WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING
#define whal_Esp32c6_Uart_Init      whal_Uart_Init
#define whal_Esp32c6_Uart_Deinit    whal_Uart_Deinit
#define whal_Esp32c6_Uart_Send      whal_Uart_Send
#define whal_Esp32c6_Uart_Recv      whal_Uart_Recv
#define whal_Esp32c6_Uart_SendAsync whal_Uart_SendAsync
#define whal_Esp32c6_Uart_RecvAsync whal_Uart_RecvAsync
#endif /* WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_ESP32C6_UART_SINGLE_INSTANCE
const whal_Uart whal_Esp32c6_Uart_Dev = WHAL_CFG_ESP32C6_UART_DEV;
#endif

whal_Error whal_Esp32c6_Uart_Init(whal_Uart *uartDev)
{
    (void)uartDev;
    return WHAL_ENOTSUP;
}

whal_Error whal_Esp32c6_Uart_Deinit(whal_Uart *uartDev)
{
    (void)uartDev;
    return WHAL_ENOTSUP;
}

whal_Error whal_Esp32c6_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    (void)uartDev;
    (void)data;
    (void)dataSz;
    return WHAL_ENOTSUP;
}

whal_Error whal_Esp32c6_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz)
{
    (void)uartDev;
    (void)data;
    (void)dataSz;
    return WHAL_ENOTSUP;
}

whal_Error whal_Esp32c6_Uart_SendAsync(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    (void)uartDev;
    (void)data;
    (void)dataSz;
    return WHAL_ENOTSUP;
}

whal_Error whal_Esp32c6_Uart_RecvAsync(whal_Uart *uartDev, void *data, size_t dataSz)
{
    (void)uartDev;
    (void)data;
    (void)dataSz;
    return WHAL_ENOTSUP;
}

#ifndef WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING
const whal_UartDriver whal_Esp32c6_Uart_Driver = {
    .Init = whal_Esp32c6_Uart_Init,
    .Deinit = whal_Esp32c6_Uart_Deinit,
    .Send = whal_Esp32c6_Uart_Send,
    .Recv = whal_Esp32c6_Uart_Recv,
    .SendAsync = whal_Esp32c6_Uart_SendAsync,
    .RecvAsync = whal_Esp32c6_Uart_RecvAsync,
};
#endif /* !WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING */
