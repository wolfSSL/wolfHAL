/* stm32wb_uart.h
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

#ifndef WHAL_STM32WB_UART_H
#define WHAL_STM32WB_UART_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/timeout.h>
/*
 * @file stm32wb_uart.h
 * @brief STM32WB UART driver — polled variant.
 */

/*
 * @brief Compute UART BRR register value.
 */
#define WHAL_STM32WB_UART_BRR(clk, baud)   ((clk) / (baud))

/*
 * @brief Compute LPUART BRR register value.
 */
#define WHAL_STM32WB_LPUART_BRR(clk, baud) ((uint32_t)(((uint64_t)(clk) * 256) / (baud)))

/* Polled UART */

/*
 * @brief Polled UART configuration.
 */
typedef struct whal_Stm32wb_Uart_Cfg {
    uint32_t brr;
    whal_Timeout *timeout;
} whal_Stm32wb_Uart_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_STM32WB_UART_DEV initializer in wolfHAL_board.h.
 */
#if defined(WHAL_CFG_STM32WB_UART_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32H5_UART_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32C0_UART_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32N6_UART_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32WBA_UART_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32WB0_UART_SINGLE_INSTANCE)
extern const whal_Uart whal_Stm32wb_Uart_Dev;
#endif

#if !defined(WHAL_CFG_STM32WB_UART_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32WB_UART_DMA_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32H5_UART_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32C0_UART_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32N6_UART_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32WBA_UART_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32WB0_UART_DIRECT_API_MAPPING)
/*
 * @brief Polled UART driver. Implements Init, Deinit, Send, Recv.
 */
extern const whal_UartDriver whal_Stm32wb_Uart_Driver;

/*
 * @brief Initialize the STM32WB UART peripheral.
 *
 * @param uartDev UART device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Uart_Init(whal_Uart *uartDev);

/*
 * @brief Deinitialize the STM32WB UART peripheral.
 *
 * @param uartDev UART device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Uart_Deinit(whal_Uart *uartDev);

/*
 * @brief Transmit a buffer over UART (blocking, polled).
 *
 * @param uartDev UART device instance.
 * @param data    Buffer to transmit.
 * @param dataSz  Number of bytes to transmit.
 *
 * @retval WHAL_SUCCESS All bytes transmitted.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz);

/*
 * @brief Receive a buffer over UART (blocking, polled).
 *
 * @param uartDev UART device instance.
 * @param data    Receive buffer.
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS All bytes received.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz);
#endif /* !WHAL_CFG_UART_API_MAPPING */

#endif /* WHAL_STM32WB_UART_H */
