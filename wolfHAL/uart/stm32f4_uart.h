/* stm32f4_uart.h
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

#ifndef WHAL_STM32F4_UART_H
#define WHAL_STM32F4_UART_H

#include <stdint.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/timeout.h>
#include <stddef.h>

/*
 * @file stm32f4_uart.h
 * @brief STM32F4 UART driver configuration.
 *
 * The STM32F4 USART uses the older USARTv1 register layout with
 * SR (Status Register) and DR (Data Register) at offsets 0x00 and 0x04,
 * which differs from the USARTv2 ISR/TDR/RDR layout used by STM32WB/H5.
 *
 * Register layout:
 *   SR  at 0x00 - Status register (TXE, RXNE, TC flags)
 *   DR  at 0x04 - Data register (shared TX/RX)
 *   BRR at 0x08 - Baud rate register
 *   CR1 at 0x0C - Control register 1 (UE, TE, RE)
 *   CR2 at 0x10 - Control register 2
 *   CR3 at 0x14 - Control register 3
 */

/*
 * @brief Compute UART BRR register value for STM32F4.
 *
 * For oversampling by 16 (OVER8=0):
 *   BRR = fPCLK / baudrate
 *
 * @param clk  Peripheral clock frequency in Hz.
 * @param baud Desired baud rate.
 */
#define WHAL_STM32F4_UART_BRR(clk, baud) ((clk) / (baud))

/*
 * @brief STM32F4 UART configuration parameters.
 */
typedef struct whal_Stm32f4_Uart_Cfg {
    uint32_t brr;
    whal_Timeout *timeout;
} whal_Stm32f4_Uart_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_STM32F4_UART_DEV initializer in wolfHAL_board.h.
 */
#if defined(WHAL_CFG_STM32F4_UART_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32L1_UART_SINGLE_INSTANCE)
extern const whal_Uart whal_Stm32f4_Uart_Dev;
#endif

#ifndef WHAL_CFG_STM32F4_UART_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32F4 UART peripheral.
 */
extern const whal_UartDriver whal_Stm32f4_Uart_Driver;

/*
 * @brief Initialize the STM32F4 UART peripheral.
 *
 * @param uartDev UART device instance to initialize.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32f4_Uart_Init(whal_Uart *uartDev);

/*
 * @brief Deinitialize the STM32F4 UART peripheral.
 *
 * @param uartDev UART device instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32f4_Uart_Deinit(whal_Uart *uartDev);

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
whal_Error whal_Stm32f4_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz);

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
whal_Error whal_Stm32f4_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz);
#endif /* !WHAL_CFG_STM32F4_UART_DIRECT_API_MAPPING */

#endif /* WHAL_STM32F4_UART_H */
