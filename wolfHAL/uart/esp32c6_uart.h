/* esp32c6_uart.h
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

#ifndef WHAL_ESP32C6_UART_H
#define WHAL_ESP32C6_UART_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/timeout.h>

/*
 * @file esp32c6_uart.h
 * @brief ESP32-C6 UART driver configuration types.
 *
 * Polled 8N1 driver. The UART core clock (UART_SCLK) source and pre-divider
 * live in PCR and are owned by the clock driver; at reset UART_SCLK is
 * XTAL_CLK (40 MHz) undivided.
 */

/*
 * @brief Compose a UART_CLKDIV_SYNC_REG value for @p baud from a UART_SCLK of
 * @p sclkHz. The divisor is CLKDIV + CLKDIV_FRAG / 16, rounded to the nearest
 * sixteenth. CLKDIV is 12 bits, so @p sclkHz / @p baud must be below 4096
 * (at 40 MHz the lowest baud rate is 9766).
 */
#define WHAL_ESP32C6_UART_CLKDIV(baud, sclkHz) \
    ((uint32_t)(((((uint64_t)(sclkHz) * 16 + (baud) / 2) / (baud)) >> 4) & 0xFFF) | \
     ((uint32_t)((((uint64_t)(sclkHz) * 16 + (baud) / 2) / (baud)) & 0xF) << 20))

/*
 * @brief UART configuration.
 */
typedef struct whal_Esp32c6_Uart_Cfg {
    uint32_t clkDiv;        /* UART_CLKDIV_SYNC_REG value, see WHAL_ESP32C6_UART_CLKDIV */
    whal_Timeout *timeout;
} whal_Esp32c6_Uart_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_ESP32C6_UART_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_ESP32C6_UART_SINGLE_INSTANCE
extern const whal_Uart whal_Esp32c6_Uart_Dev;
#endif

#ifndef WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING
/*
 * @brief Driver instance for ESP32-C6 UART.
 */
extern const whal_UartDriver whal_Esp32c6_Uart_Driver;

/*
 * @brief Initialize the ESP32-C6 UART peripheral.
 *
 * @param uartDev UART device instance.
 *
 * @retval WHAL_SUCCESS  Initialization completed.
 * @retval WHAL_EINVAL   Invalid arguments.
 * @retval WHAL_ETIMEOUT Pending transmit or register sync did not finish.
 */
whal_Error whal_Esp32c6_Uart_Init(whal_Uart *uartDev);

/*
 * @brief Deinitialize the ESP32-C6 UART peripheral.
 *
 * @param uartDev UART device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Esp32c6_Uart_Deinit(whal_Uart *uartDev);

/*
 * @brief Transmit a buffer over UART.
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to transmit.
 * @param dataSz  Number of bytes to transmit.
 *
 * @retval WHAL_SUCCESS  All bytes transmitted.
 * @retval WHAL_EINVAL   Invalid arguments.
 * @retval WHAL_ETIMEOUT FIFO space or transmit completion did not arrive.
 */
whal_Error whal_Esp32c6_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz);

/*
 * @brief Receive a buffer over UART.
 *
 * @param uartDev UART device instance.
 * @param data    Receive buffer.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS   All bytes received.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ETIMEOUT  A byte did not arrive.
 * @retval WHAL_EHARDWARE A parity, framing or RX FIFO overflow error was
 *                        detected.
 */
whal_Error whal_Esp32c6_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz);

/*
 * @brief Start an asynchronous transmit.
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to transmit. Must remain valid until completion.
 * @param dataSz  Number of bytes to transmit.
 *
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Async transfers are not supported.
 */
whal_Error whal_Esp32c6_Uart_SendAsync(whal_Uart *uartDev, const void *data, size_t dataSz);

/*
 * @brief Start an asynchronous receive.
 *
 * @param uartDev UART device instance.
 * @param data    Receive buffer. Must remain valid until completion.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Async transfers are not supported.
 */
whal_Error whal_Esp32c6_Uart_RecvAsync(whal_Uart *uartDev, void *data, size_t dataSz);
#endif /* !WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING */

#endif /* WHAL_ESP32C6_UART_H */
