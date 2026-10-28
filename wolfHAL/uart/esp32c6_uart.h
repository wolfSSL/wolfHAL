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

/*
 * @file esp32c6_uart.h
 * @brief ESP32-C6 UART driver configuration types (stub).
 */

/*
 * @brief UART configuration. The field set is expected to grow as the driver
 * is built out (clock source, data bits, parity, stop bits, pins).
 */
typedef struct whal_Esp32c6_Uart_Cfg {
    uint32_t baud;  /* Target baud rate */
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
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Uart_Init(whal_Uart *uartDev);

/*
 * @brief Deinitialize the ESP32-C6 UART peripheral.
 *
 * @param uartDev UART device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Uart_Deinit(whal_Uart *uartDev);

/*
 * @brief Transmit a buffer over UART.
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to transmit.
 * @param dataSz  Number of bytes to transmit.
 *
 * @retval WHAL_SUCCESS All bytes transmitted.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz);

/*
 * @brief Receive a buffer over UART.
 *
 * @param uartDev UART device instance.
 * @param data    Receive buffer.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS All bytes received.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz);

/*
 * @brief Start an asynchronous transmit.
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to transmit. Must remain valid until completion.
 * @param dataSz  Number of bytes to transmit.
 *
 * @retval WHAL_SUCCESS Transfer started.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Uart_SendAsync(whal_Uart *uartDev, const void *data, size_t dataSz);

/*
 * @brief Start an asynchronous receive.
 *
 * @param uartDev UART device instance.
 * @param data    Receive buffer. Must remain valid until completion.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS Receive started.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Uart_RecvAsync(whal_Uart *uartDev, void *data, size_t dataSz);
#endif /* !WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING */

#endif /* WHAL_ESP32C6_UART_H */
