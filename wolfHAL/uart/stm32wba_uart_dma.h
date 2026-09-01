/* stm32wba_uart_dma.h
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

#ifndef WHAL_STM32WBA_UART_DMA_H
#define WHAL_STM32WBA_UART_DMA_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/uart/stm32wb_uart.h>
#include <wolfHAL/dma/dma.h>
#include <wolfHAL/dma/stm32wba_gpdma.h>

/*
 * @file stm32wba_uart_dma.h
 * @brief STM32WBA UART driver -- GPDMA-backed variant.
 *
 * Uses the STM32WBA GPDMA controller to move bytes between memory and the
 * USART TDR/RDR registers. Init/Deinit reuse the polled UART driver; Send/Recv
 * block using DMA; SendAsync/RecvAsync return immediately.
 *
 * The GPDMA request lines for USART are (RM0493 Table 208):
 *   REQSEL 11 = USART1_RX
 *   REQSEL 12 = USART1_TX
 *   REQSEL 13 = USART2_RX
 *   REQSEL 14 = USART2_TX
 */

/*
 * @brief Configuration for the GPDMA-backed STM32WBA UART driver.
 *
 * `base` is reused by the polled UART driver for register-level setup
 * (BRR, timeout). The DMA fields specify which GPDMA channels carry
 * TX/RX traffic for this UART instance.
 */
typedef struct {
    whal_Stm32wb_Uart_Cfg base;             /* Polled-UART base config */
    whal_Dma *dma;                          /* GPDMA controller instance */
    size_t txCh;                            /* GPDMA channel for TX */
    size_t rxCh;                            /* GPDMA channel for RX */
    whal_Stm32wba_Gpdma_ChCfg *txChCfg;     /* TX channel cfg (REQSEL etc.) */
    whal_Stm32wba_Gpdma_ChCfg *rxChCfg;     /* RX channel cfg */
    volatile whal_Error txResult;           /* Set by TX completion callback */
    volatile whal_Error rxResult;           /* Set by RX completion callback */
} whal_Stm32wba_UartDma_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_STM32WBA_UART_DMA_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_STM32WBA_UART_DMA_SINGLE_INSTANCE
extern const whal_Uart whal_Stm32wba_UartDma_Dev;
#endif

/*
 * @brief Driver instance for the GPDMA-backed STM32WBA UART.
 */
extern const whal_UartDriver whal_Stm32wba_UartDma_Driver;

/*
 * @brief Send `dataSz` bytes via GPDMA. Blocks until the transfer completes.
 *
 * @param uartDev UART device instance.
 * @param data    Source buffer (must be DMA-accessible).
 * @param dataSz  Number of bytes to send.
 *
 * @retval WHAL_SUCCESS   All bytes sent.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ETIMEOUT  DMA transfer did not complete within the timeout.
 * @retval WHAL_EHARDWARE GPDMA reported a transfer error.
 */
whal_Error whal_Stm32wba_UartDma_Send(whal_Uart *uartDev, const void *data,
                                      size_t dataSz);

/*
 * @brief Receive `dataSz` bytes via GPDMA. Blocks until the transfer completes.
 *
 * @param uartDev UART device instance.
 * @param data    Destination buffer (must be DMA-accessible).
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS   All bytes received.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ETIMEOUT  DMA transfer did not complete within the timeout.
 * @retval WHAL_EHARDWARE GPDMA reported a transfer error.
 */
whal_Error whal_Stm32wba_UartDma_Recv(whal_Uart *uartDev, void *data,
                                      size_t dataSz);

/*
 * @brief Start a non-blocking GPDMA TX transfer. Returns once the transfer
 *        has been queued; completion is reported via the TX callback.
 *
 * @param uartDev UART device instance.
 * @param data    Source buffer (must be DMA-accessible until completion).
 * @param dataSz  Number of bytes to send.
 *
 * @retval WHAL_SUCCESS   Transfer started.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ENOTREADY A previous TX is still in flight.
 */
whal_Error whal_Stm32wba_UartDma_SendAsync(whal_Uart *uartDev, const void *data,
                                           size_t dataSz);

/*
 * @brief Start a non-blocking GPDMA RX transfer. Returns once the transfer
 *        has been queued; completion is reported via the RX callback.
 *
 * @param uartDev UART device instance.
 * @param data    Destination buffer (must be DMA-accessible until completion).
 * @param dataSz  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS   Transfer started.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ENOTREADY A previous RX is still in flight.
 */
whal_Error whal_Stm32wba_UartDma_RecvAsync(whal_Uart *uartDev, void *data,
                                           size_t dataSz);

/*
 * @brief GPDMA TX channel completion callback. Boards wire this into the
 *        appropriate GPDMA channel IRQ handler. The board must pass the
 *        owning UART's `cfg` pointer as `ctx`.
 *
 * @param ctx Pointer to the UART instance's `whal_Stm32wba_UartDma_Cfg`.
 * @param err Result reported by the GPDMA driver for the completed transfer.
 */
void whal_Stm32wba_UartDma_TxCallback(void *ctx, whal_Error err);

/*
 * @brief GPDMA RX channel completion callback. See `TxCallback` for usage.
 *
 * @param ctx Pointer to the UART instance's `whal_Stm32wba_UartDma_Cfg`.
 * @param err Result reported by the GPDMA driver for the completed transfer.
 */
void whal_Stm32wba_UartDma_RxCallback(void *ctx, whal_Error err);

#endif /* WHAL_STM32WBA_UART_DMA_H */
