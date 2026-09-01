/* stm32wb_dma.h
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

#ifndef WHAL_STM32WB_DMA_H
#define WHAL_STM32WB_DMA_H

#include <stddef.h>
#include <stdint.h>
#include <wolfHAL/dma/dma.h>
#include <wolfHAL/reg.h>

/*
 * @file stm32wb_dma.h
 * @brief STM32WB DMA driver configuration types.
 *
 * The STM32WB has two DMA controllers:
 * - DMA1 with 7 channels
 * - DMA2 with 5 channels
 *
 * Each controller is represented by a single whal_Dma instance. Channels are
 * identified by a 0-based index (0-6 for DMA1, 0-4 for DMA2).
 *
 * A DMAMUX peripheral maps peripheral request lines to DMA channels.
 * DMAMUX channels 0-6 correspond to DMA1 channels 1-7. DMAMUX channels 7-11
 * correspond to DMA2 channels 1-5. The driver handles this mapping internally;
 * the caller only needs to provide the DMAREQ_ID for the desired peripheral.
 */

/*
 * @brief Transfer direction.
 */
typedef enum {
    WHAL_STM32WB_DMA_DIR_PERIPH_TO_MEM, /* Peripheral to memory */
    WHAL_STM32WB_DMA_DIR_MEM_TO_PERIPH, /* Memory to peripheral */
    WHAL_STM32WB_DMA_DIR_MEM_TO_MEM,    /* Memory to memory */
} whal_Stm32wb_Dma_Dir;

/*
 * @brief Data width for transfers.
 */
typedef enum {
    WHAL_STM32WB_DMA_WIDTH_8BIT,  /* 8-bit (byte) */
    WHAL_STM32WB_DMA_WIDTH_16BIT, /* 16-bit (half-word) */
    WHAL_STM32WB_DMA_WIDTH_32BIT, /* 32-bit (word) */
} whal_Stm32wb_Dma_Width;

/*
 * @brief Address increment mode.
 */
typedef enum {
    WHAL_STM32WB_DMA_INC_DISABLE, /* Fixed address */
    WHAL_STM32WB_DMA_INC_ENABLE,  /* Increment address after each transfer */
} whal_Stm32wb_Dma_Inc;

/*
 * @brief Per-channel DMA configuration.
 */
typedef struct {
    whal_Stm32wb_Dma_Dir dir;       /* Transfer direction */
    uint32_t srcAddr;              /* Source address */
    uint32_t dstAddr;              /* Destination address */
    uint16_t length;               /* Number of data items to transfer */
    whal_Stm32wb_Dma_Width width;   /* Data width (8, 16, or 32 bit) */
    whal_Stm32wb_Dma_Inc srcInc;    /* Source address increment mode */
    whal_Stm32wb_Dma_Inc dstInc;    /* Destination address increment mode */
    uint8_t dmamuxReqId;           /* DMAMUX request ID for the peripheral */
    uint8_t circular;              /* Non-zero to enable circular mode */
} whal_Stm32wb_Dma_ChCfg;

/*
 * @brief Controller-level DMA configuration.
 *
 * Contains the DMAMUX base address and the DMAMUX channel offset for this
 * controller. DMA1 uses DMAMUX channels starting at 0; DMA2 starts at 7.
 */
typedef struct {
    size_t dmamuxBase;       /* DMAMUX1 register base address */
    uint8_t dmamuxChOffset;  /* First DMAMUX channel index for this controller */
    uint8_t numChannels;     /* Number of channels (7 for DMA1, 5 for DMA2) */
} whal_Stm32wb_Dma_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_STM32WB_DMA_DEV initializer in wolfHAL_board.h.
 */
#if defined(WHAL_CFG_STM32WB_DMA_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32WB0_DMA_SINGLE_INSTANCE)
extern const whal_Dma whal_Stm32wb_Dma_Dev;
#endif

#if !defined(WHAL_CFG_STM32WB_DMA_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32WB0_DMA_DIRECT_API_MAPPING)
/*
 * @brief Driver instance for STM32WB DMA.
 */
extern const whal_DmaDriver whal_Stm32wb_Dma_Driver;

/*
 * @brief Initialize the STM32WB DMA controller.
 */
whal_Error whal_Stm32wb_Dma_Init(whal_Dma *dmaDev);

/*
 * @brief Deinitialize the STM32WB DMA controller.
 */
whal_Error whal_Stm32wb_Dma_Deinit(whal_Dma *dmaDev);

/*
 * @brief Configure a DMA channel with platform-specific settings.
 */
whal_Error whal_Stm32wb_Dma_Configure(whal_Dma *dmaDev, size_t ch,
                                     const void *chCfg);

/*
 * @brief Start a previously configured DMA channel.
 */
whal_Error whal_Stm32wb_Dma_Start(whal_Dma *dmaDev, size_t ch);

/*
 * @brief Stop a DMA channel.
 */
whal_Error whal_Stm32wb_Dma_Stop(whal_Dma *dmaDev, size_t ch);
#endif /* !WHAL_CFG_STM32WB_DMA_DIRECT_API_MAPPING */

/*
 * @brief DMA callback type.
 *
 * Called from ISR context when a transfer completes or an error occurs.
 *
 * @param ctx User context pointer.
 * @param err WHAL_SUCCESS on transfer complete, WHAL_EHARDWARE on error.
 */
typedef void (*whal_Stm32wb_Dma_Callback)(void *ctx, whal_Error err);

/*
 * @brief Handle a DMA channel interrupt.
 *
 * Checks TCIF/TEIF flags, clears them, and invokes the callback.
 * Should be called from the board's ISR entry point.
 *
 * @param dmaDev DMA controller instance.
 * @param ch     Channel index (0-based).
 * @param cb     Callback to invoke (or NULL to just clear flags).
 * @param ctx    Context pointer passed to callback.
 */
void whal_Stm32wb_Dma_IRQHandler(whal_Dma *dmaDev, size_t ch,
                                 whal_Stm32wb_Dma_Callback cb, void *ctx);

#endif /* WHAL_STM32WB_DMA_H */
