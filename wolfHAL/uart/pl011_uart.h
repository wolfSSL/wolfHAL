/* pl011_uart.h
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

#ifndef WHAL_PL011_UART_H
#define WHAL_PL011_UART_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/timeout.h>
/*
 * @file pl011_uart.h
 * @brief Arm PL011 UART driver — polled variant.
 *
 * Used on AMD Versal Gen 2 (AM026 ch. 63, register map in AM028 UART
 * module).
 */

/*
 * @brief Baud divisor times 64, rounded: round(clk * 4 / baud).
 *
 * Divisor = clk / (16 * baud), 6-bit fraction (AM026 ch. 63 p670). A
 * fraction that rounds up to 64 carries into the whole part.
 */
#define WHAL_PL011_UART_DIV64(clk, baud) \
    ((((uint64_t)(clk) * 8) / (baud) + 1) / 2)

/*
 * @brief Integer baud divisor (IBRD / BAUD_INTEGER) value.
 */
#define WHAL_PL011_UART_IBRD(clk, baud) \
    ((uint32_t)(WHAL_PL011_UART_DIV64(clk, baud) >> 6))

/*
 * @brief Fractional baud divisor (FBRD / BAUD_FRACT) value.
 */
#define WHAL_PL011_UART_FBRD(clk, baud) \
    ((uint32_t)(WHAL_PL011_UART_DIV64(clk, baud) & 0x3F))

/*
 * @brief LCR_H (LINE_CTRL) bits (AM028 UART LINE_CTRL).
 */
#define WHAL_PL011_UART_LCRH_PEN    (1UL << 1) /* parity on */
#define WHAL_PL011_UART_LCRH_EPS    (1UL << 2) /* even parity */
#define WHAL_PL011_UART_LCRH_STP2   (1UL << 3) /* two stop bits */
#define WHAL_PL011_UART_LCRH_FEN    (1UL << 4) /* FIFOs on */
#define WHAL_PL011_UART_LCRH_WLEN_8 (3UL << 5) /* 8 data bits */

/*
 * @brief Default LCR_H value: 8 data bits, no parity, 1 stop bit, FIFOs on.
 */
#define WHAL_PL011_UART_LCRH_8N1_FIFO \
    (WHAL_PL011_UART_LCRH_WLEN_8 | WHAL_PL011_UART_LCRH_FEN)

/* Polled UART */

/*
 * @brief Polled UART configuration.
 *
 * ibrd and fbrd are computed by the board, e.g. with
 * WHAL_PL011_UART_IBRD(clk, baud) and WHAL_PL011_UART_FBRD(clk, baud).
 */
typedef struct whal_Pl011_Uart_Cfg {
    uint32_t ibrd;
    uint32_t fbrd;
    uint32_t lcrH;
    whal_Timeout *timeout;
} whal_Pl011_Uart_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_PL011_UART_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_PL011_UART_SINGLE_INSTANCE
extern const whal_Uart whal_Pl011_Uart_Dev;
#endif

#ifndef WHAL_CFG_PL011_UART_DIRECT_API_MAPPING
/*
 * @brief Polled UART driver. Implements Init, Deinit, Send, Recv.
 */
extern const whal_UartDriver whal_Pl011_Uart_Driver;

/*
 * @brief Initialize the PL011 UART.
 *
 * @param uartDev UART device instance.
 *
 * If the UART is on, bytes left in the TX FIFO go out first; if it is
 * off, they are flushed. ibrd 0xFFFF needs fbrd 0.
 *
 * @retval WHAL_SUCCESS  Initialization completed.
 * @retval WHAL_EINVAL   Invalid arguments, or ibrd / fbrd out of range.
 * @retval WHAL_ETIMEOUT The UART stayed busy.
 */
whal_Error whal_Pl011_Uart_Init(whal_Uart *uartDev);

/*
 * @brief Deinitialize the PL011 UART.
 *
 * @param uartDev UART device instance.
 *
 * Waits for queued bytes to go out (flushed if the UART is off) before
 * clearing the setup.
 *
 * @retval WHAL_SUCCESS  Deinit completed.
 * @retval WHAL_EINVAL   Invalid arguments.
 * @retval WHAL_ETIMEOUT The UART stayed busy.
 */
whal_Error whal_Pl011_Uart_Deinit(whal_Uart *uartDev);

/*
 * @brief Transmit a buffer over UART (blocking, polled).
 *
 * Bytes are sent as they are; no '\n' to "\r\n" change is made.
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to transmit.
 * @param dataSz  Number of bytes to transmit.
 *
 * @retval WHAL_SUCCESS  All bytes transmitted.
 * @retval WHAL_EINVAL   Invalid arguments.
 * @retval WHAL_ETIMEOUT The TX FIFO did not drain in time.
 */
whal_Error whal_Pl011_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz);

/*
 * @brief Receive a buffer over UART (blocking, polled).
 *
 * @param uartDev UART device instance.
 * @param data    Receive buffer.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS   All bytes received.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ETIMEOUT  No byte came in time.
 * @retval WHAL_EHARDWARE An overrun, break, parity or framing error was
 *                        detected. The bad byte is dropped.
 */
whal_Error whal_Pl011_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz);
#endif /* !WHAL_CFG_PL011_UART_DIRECT_API_MAPPING */

#endif /* WHAL_PL011_UART_H */
