/* stm32f0_uart.h
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

#ifndef WHAL_STM32F0_UART_H
#define WHAL_STM32F0_UART_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32f0_uart.h
 * @brief STM32F0 UART driver — polled variant.
 *
 * The STM32F0 USART uses the same ISR/TDR/RDR register layout as the
 * STM32WB but does not have the FIFOEN bit in CR1 (no hardware FIFO).
 */

#define WHAL_STM32F0_UART_BRR(clk, baud) ((clk) / (baud))

typedef struct whal_Stm32f0_Uart_Cfg {
    uint32_t brr;
    whal_Timeout *timeout;
} whal_Stm32f0_Uart_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_STM32F0_UART_DEV initializer in wolfHAL_board.h.
 */
#if defined(WHAL_CFG_STM32F0_UART_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32F3_UART_SINGLE_INSTANCE)
extern const whal_Uart whal_Stm32f0_Uart_Dev;
#endif

#ifndef WHAL_CFG_STM32F0_UART_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the STM32F0 polled UART.
 */
extern const whal_UartDriver whal_Stm32f0_Uart_Driver;

/*
 * @brief Initialize the STM32F0 UART (configure BRR, enable TX/RX/USART).
 *
 * @param uartDev UART device instance.
 *
 * @retval WHAL_SUCCESS UART is ready for Send/Recv.
 * @retval WHAL_EINVAL  Null pointer or missing cfg.
 */
whal_Error whal_Stm32f0_Uart_Init(whal_Uart *uartDev);

/*
 * @brief Deinitialize the STM32F0 UART (disable TX/RX/USART).
 *
 * @param uartDev UART device instance.
 *
 * @retval WHAL_SUCCESS UART has been disabled.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Stm32f0_Uart_Deinit(whal_Uart *uartDev);

/*
 * @brief Send `dataSz` bytes from `data`, polling TXE between bytes.
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to send.
 * @param dataSz  Number of bytes to send.
 *
 * @retval WHAL_SUCCESS All bytes sent.
 * @retval WHAL_EINVAL  Null pointer.
 * @retval WHAL_ETIMEOUT Hardware did not assert TXE within the configured timeout.
 */
whal_Error whal_Stm32f0_Uart_Send(whal_Uart *uartDev, const void *data,
                                  size_t dataSz);

/*
 * @brief Receive `dataSz` bytes into `data`, polling RXNE between bytes.
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to receive into.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS All bytes received.
 * @retval WHAL_EINVAL  Null pointer.
 * @retval WHAL_ETIMEOUT Hardware did not assert RXNE within the configured timeout.
 */
whal_Error whal_Stm32f0_Uart_Recv(whal_Uart *uartDev, void *data,
                                  size_t dataSz);
#endif /* !WHAL_CFG_STM32F0_UART_DIRECT_API_MAPPING */

#endif /* WHAL_STM32F0_UART_H */
