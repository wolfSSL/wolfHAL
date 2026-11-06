/* uart.h
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

#ifndef WHAL_UART_H
#define WHAL_UART_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>

/*
 * @file uart.h
 * @brief Generic UART abstraction and driver interface.
 */

typedef struct whal_Uart whal_Uart;

/*
 * @brief Driver vtable for UART devices.
 */
typedef struct {
    /* Initialize the UART hardware. */
    whal_Error (*Init)(whal_Uart *uartDev);
    /* Deinitialize the UART hardware. */
    whal_Error (*Deinit)(whal_Uart *uartDev);
    /* Transmit a buffer. */
    whal_Error (*Send)(whal_Uart *uartDev, const void *data, size_t dataSz);
    /* Receive into a buffer. */
    whal_Error (*Recv)(whal_Uart *uartDev, void *data, size_t dataSz);
    /* Start an asynchronous transmit. NULL if not supported. */
    whal_Error (*SendAsync)(whal_Uart *uartDev, const void *data, size_t dataSz);
    /* Start an asynchronous receive. NULL if not supported. */
    whal_Error (*RecvAsync)(whal_Uart *uartDev, void *data, size_t dataSz);
} whal_UartDriver;

/*
 * @brief UART device instance with a base address, driver vtable, and platform-specific cfg.
 */
struct whal_Uart {
    const uintptr_t base;
    const whal_UartDriver *driver;
    void *cfg;
};

/*
 * @brief Initializes a UART device and its driver.
 *
 * @param uartDev Pointer to the UART instance to initialize.
 *
 * @retval WHAL_SUCCESS  Driver-specific init completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Uart_Init(whal_Uart *uartDev);

/*
 * @brief Deinitializes a UART device and releases resources.
 *
 * @param uartDev Pointer to the UART instance to deinitialize.
 *
 * @retval WHAL_SUCCESS  Driver-specific deinit completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Uart_Deinit(whal_Uart *uartDev);

/*
 * @brief Sends a buffer over the UART.
 *
 * @param uartDev Pointer to the UART instance.
 * @param data    Buffer to transmit.
 * @param dataSz  Number of bytes to send.
 *
 * @retval WHAL_SUCCESS  Buffer was queued or transmitted.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz);

/*
 * @brief Receives data from the UART into a buffer.
 *
 * @param uartDev Pointer to the UART instance.
 * @param data    Destination buffer.
 * @param dataSz  Maximum number of bytes to read.
 *
 * @retval WHAL_SUCCESS  Buffer was filled or receive started.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 * @retval WHAL_EHARDWARE A receive line error (parity, framing, noise or
 *                        overrun) was detected.
 */
whal_Error whal_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz);

/*
 * @brief Start an asynchronous transmit.
 *
 * Returns immediately. The buffer must remain valid until the transfer
 * completes. Completion signaling is driver-specific (e.g., a status
 * field in the driver config).
 *
 * @param uartDev Pointer to the UART instance.
 * @param data    Buffer to transmit. Must remain valid until completion.
 * @param dataSz  Number of bytes to send.
 *
 * @retval WHAL_SUCCESS   Transfer started.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ENOTSUP  Async not supported by this driver.
 */
whal_Error whal_Uart_SendAsync(whal_Uart *uartDev, const void *data, size_t dataSz);

/*
 * @brief Start an asynchronous receive.
 *
 * Returns immediately. The buffer must remain valid until the transfer
 * completes. Completion signaling is driver-specific (e.g., a status
 * field in the driver config).
 *
 * @param uartDev Pointer to the UART instance.
 * @param data    Receive buffer. Must remain valid until completion.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS   Receive started.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ENOTSUP  Async not supported by this driver.
 */
whal_Error whal_Uart_RecvAsync(whal_Uart *uartDev, void *data, size_t dataSz);

#endif /* WHAL_UART_H */
