/* pic32cz_uart.h
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

#ifndef WHAL_PIC32CZ_UART_H
#define WHAL_PIC32CZ_UART_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/timeout.h>

/*
 * @file pic32cz_uart.h
 * @brief PIC32CZ SERCOM USART driver configuration.
 */

#define WHAL_PIC32CZ_UART_BAUD(freq_baud, freq_ref) \
    (65536UL - (65536UL * 16UL * (uint64_t)(freq_baud) / (uint64_t)(freq_ref)))

/*
 * @brief PIC32CZ SERCOM USART TX pad output options.
 */
typedef enum {
    WHAL_PIC32CZ_UART_TXPO_PAD0 = 0x0,       /* TxD on PAD[0], XCK on PAD[1] */
    WHAL_PIC32CZ_UART_TXPO_PAD0_RTS_CTS = 0x2, /* TxD on PAD[0], RTS on PAD[2], CTS on PAD[3] */
    WHAL_PIC32CZ_UART_TXPO_PAD0_TE = 0x3,    /* TxD on PAD[0], TE on PAD[2] */
} whal_Pic32cz_Uart_TxPad;

/*
 * @brief PIC32CZ SERCOM USART RX pad input options.
 */
typedef enum {
    WHAL_PIC32CZ_UART_RXPO_PAD0 = 0x0,
    WHAL_PIC32CZ_UART_RXPO_PAD1 = 0x1,
    WHAL_PIC32CZ_UART_RXPO_PAD2 = 0x2,
    WHAL_PIC32CZ_UART_RXPO_PAD3 = 0x3,
} whal_Pic32cz_Uart_RxPad;

/*
 * @brief PIC32CZ SERCOM USART configuration parameters.
 */
typedef struct whal_Pic32cz_Uart_Cfg {
    uint32_t baud;
    whal_Pic32cz_Uart_TxPad txPad;
    whal_Pic32cz_Uart_RxPad rxPad;
    whal_Timeout *timeout;
} whal_Pic32cz_Uart_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_PIC32CZ_UART_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_PIC32CZ_UART_SINGLE_INSTANCE
extern const whal_Uart whal_Pic32cz_Uart_Dev;
#endif

#ifndef WHAL_CFG_PIC32CZ_UART_DIRECT_API_MAPPING
/*
 * @brief Driver instance for PIC32CZ UART.
 */
extern const whal_UartDriver whal_Pic32cz_Uart_Driver;

/*
 * @brief Initialize the PIC32CZ UART peripheral.
 *
 * @param uartDev UART device instance to initialize.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Uart_Init(whal_Uart *uartDev);
/*
 * @brief Deinitialize the PIC32CZ UART peripheral.
 *
 * @param uartDev UART device instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Uart_Deinit(whal_Uart *uartDev);
/*
 * @brief Transmit a buffer over UART.
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to transmit.
 * @param dataSz  Number of bytes to transmit.
 *
 * @retval WHAL_SUCCESS Transfer completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz);
/*
 * @brief Receive a buffer over UART.
 *
 * @param uartDev UART device instance.
 * @param data    Receive buffer.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS Transfer completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz);
#endif /* !WHAL_CFG_PIC32CZ_UART_DIRECT_API_MAPPING */

#endif /* WHAL_PIC32CZ_UART_H */
