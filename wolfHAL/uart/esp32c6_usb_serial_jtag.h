/* esp32c6_usb_serial_jtag.h
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

#ifndef WHAL_ESP32C6_USB_SERIAL_JTAG_H
#define WHAL_ESP32C6_USB_SERIAL_JTAG_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/timeout.h>

/*
 * @file esp32c6_usb_serial_jtag.h
 * @brief ESP32-C6 USB Serial/JTAG controller as a UART.
 *
 * The controller (TRM chapter 32) is a fixed-function USB device that the
 * host sees as a CDC-ACM serial port plus a JTAG adapter. Firmware moves
 * serial data through a 64-byte FIFO in each direction; the line coding the
 * host sets has no effect.
 *
 * The ROM leaves the controller clocked and running, and the JTAG side shares
 * it, so Init and Deinit leave the clock and reset alone. Data written while
 * no host program has the port open is never read; Send then returns
 * WHAL_ETIMEOUT once the cfg timeout expires.
 */

/*
 * @brief USB Serial/JTAG configuration.
 */
typedef struct {
    whal_Timeout *timeout; /* Wait for FIFO space or data, per byte */
} whal_Esp32c6_UsbSerialJtag_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_ESP32C6_USB_SERIAL_JTAG_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Uart whal_Esp32c6_UsbSerialJtag_Dev;

/*
 * @brief Driver instance for the ESP32-C6 USB Serial/JTAG controller.
 */
extern const whal_UartDriver whal_Esp32c6_UsbSerialJtag_Driver;

/*
 * @brief No-op; the ROM leaves the controller running.
 *
 * @param uartDev UART device instance.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_UsbSerialJtag_Init(whal_Uart *uartDev);

/*
 * @brief No-op; the controller stays running for JTAG.
 *
 * @param uartDev UART device instance.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_UsbSerialJtag_Deinit(whal_Uart *uartDev);

/*
 * @brief Send a buffer to the host and flush it.
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to send.
 * @param dataSz  Number of bytes to send.
 *
 * @retval WHAL_SUCCESS  All bytes queued for the host.
 * @retval WHAL_EINVAL   Invalid arguments.
 * @retval WHAL_ETIMEOUT The host stopped reading the FIFO.
 */
whal_Error whal_Esp32c6_UsbSerialJtag_Send(whal_Uart *uartDev, const void *data,
                                           size_t dataSz);

/*
 * @brief Receive a buffer from the host.
 *
 * @param uartDev UART device instance.
 * @param data    Receive buffer.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS  All bytes received.
 * @retval WHAL_EINVAL   Invalid arguments.
 * @retval WHAL_ETIMEOUT A byte did not arrive.
 */
whal_Error whal_Esp32c6_UsbSerialJtag_Recv(whal_Uart *uartDev, void *data,
                                           size_t dataSz);

/*
 * @brief Not supported.
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to send.
 * @param dataSz  Number of bytes to send.
 *
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Always otherwise.
 */
whal_Error whal_Esp32c6_UsbSerialJtag_SendAsync(whal_Uart *uartDev,
                                                const void *data,
                                                size_t dataSz);

/*
 * @brief Not supported.
 *
 * @param uartDev UART device instance.
 * @param data    Receive buffer.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Always otherwise.
 */
whal_Error whal_Esp32c6_UsbSerialJtag_RecvAsync(whal_Uart *uartDev, void *data,
                                                size_t dataSz);

#endif /* WHAL_ESP32C6_USB_SERIAL_JTAG_H */
