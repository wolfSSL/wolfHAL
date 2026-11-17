/* versal2_ospi_dma.h
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

#ifndef WHAL_VERSAL2_OSPI_DMA_H
#define WHAL_VERSAL2_OSPI_DMA_H

#include <stddef.h>
#include <stdint.h>
#include <wolfHAL/error.h>
#include <wolfHAL/timeout.h>
#include <wolfHAL/dma/dma.h>

/*
 * @file versal2_ospi_dma.h
 * @brief AMD Versal Gen 2 OSPI DMA (whal_Dma driver), polled.
 *
 * The destination DMA that moves OSPI indirect read data to memory (AM026
 * p706-709). One channel (0). The whal_Dma base is the OSPI base (DMA
 * blocks at +0x1000 / +0x1800). chCfg for Configure is a
 * whal_DmaMemXfer: buffer and length, 4-byte aligned.
 */

/* Source read address = IND_AHB_ADDR_TRIG (AM026 p707 step 2b), the flash
 * window start (p704). */
#define WHAL_VERSAL2_OSPI_DMA_SRC_RD_ADDR 0xC0000000UL

/* DST_CTRL: reset value 0x803FFA00 (AM028) with SSS_FIFOTHRESH (31:25) at
 * its max 0x7A, for the AM026 Table 223 settings (p707-708). */
#define WHAL_VERSAL2_OSPI_DMA_DST_CTRL    0xF43FFA00UL

/* The only channel. */
#define WHAL_VERSAL2_OSPI_DMA_CH 0

/* Largest transfer in bytes: the SIZE field is bits 28:2, and bits 31:29
 * ignore writes (AM028 OSPIDMA_DST_SIZE). */
#define WHAL_VERSAL2_OSPI_DMA_MAX_LEN 0x1FFFFFFCUL

/*
 * @brief OSPI DMA configuration (the whal_Dma cfg).
 */
typedef struct {
    uint32_t srcRdAddr;      /* DMA_SRC_RD_ADDR (AMD: 0xC0000000) */
    uint32_t dstCtrl;        /* DMA_DST_CTRL (AMD: 0xF43FFA00) */
    void (*CacheCleanInval)(void *buf, size_t len);   /* before (optional) */
    void (*CacheInval)(void *buf, size_t len);        /* after (optional) */
    whal_Timeout *timeout;   /* bound for Wait; NULL = wait forever */
    /* State: the transfer Configure set, used by Start and Wait. */
    whal_DmaMemXfer *xfer;
} whal_Versal2_OspiDma_Cfg;

#ifndef WHAL_CFG_VERSAL2_OSPI_DMA_DIRECT_API_MAPPING
/*
 * @brief OSPI DMA driver: Init, Deinit, Configure, Start, Stop, Wait.
 */
extern const whal_DmaDriver whal_Versal2_OspiDma_Driver;

/*
 * @brief Clear old status bits (W1C).
 *
 * @param dmaDev DMA instance.
 *
 * @retval WHAL_SUCCESS Done.
 * @retval WHAL_EINVAL  Null pointer or no cfg / xfer.
 */
whal_Error whal_Versal2_OspiDma_Init(whal_Dma *dmaDev);

/*
 * @brief Nothing to undo.
 *
 * @param dmaDev DMA instance.
 *
 * @retval WHAL_SUCCESS Done.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Versal2_OspiDma_Deinit(whal_Dma *dmaDev);

/*
 * @brief Set up one transfer into a buffer.
 *
 * Cleans the buffer cache lines, then writes the destination address,
 * source and control (AM026 p707 steps 2a-2d). Start sends it.
 *
 * @param dmaDev DMA instance.
 * @param ch     WHAL_VERSAL2_OSPI_DMA_CH.
 * @param chCfg  A whal_DmaMemXfer: buf 4-byte aligned, len not 0, a
 *               multiple of 4, at most WHAL_VERSAL2_OSPI_DMA_MAX_LEN.
 *
 * @retval WHAL_SUCCESS Configured.
 * @retval WHAL_EINVAL  Null pointer, bad channel, NULL buf, len 0, buf or
 *                      len not 4-byte aligned, or len too large.
 * @retval WHAL_ENOTREADY The channel is still busy (AM028 DST_STS BUSY).
 */
whal_Error whal_Versal2_OspiDma_Configure(whal_Dma *dmaDev, size_t ch,
                                          const void *chCfg);

/*
 * @brief Start the configured transfer: the DST_SIZE write starts it
 * (AM026 p707).
 *
 * @param dmaDev DMA instance.
 * @param ch     WHAL_VERSAL2_OSPI_DMA_CH.
 *
 * @retval WHAL_SUCCESS Started.
 * @retval WHAL_EINVAL  Null pointer, bad channel, or not configured.
 */
whal_Error whal_Versal2_OspiDma_Start(whal_Dma *dmaDev, size_t ch);

/*
 * @brief Clear the status bits (W1C) and the finished-transfer count.
 * AM026/AM028 give no abort: a running transfer must finish.
 *
 * @param dmaDev DMA instance.
 * @param ch     WHAL_VERSAL2_OSPI_DMA_CH.
 *
 * @retval WHAL_SUCCESS   Done; the channel is idle.
 * @retval WHAL_EINVAL    Null pointer or bad channel.
 * @retval WHAL_ENOTREADY A transfer is still running (DST_STS BUSY).
 */
whal_Error whal_Versal2_OspiDma_Stop(whal_Dma *dmaDev, size_t ch);

/*
 * @brief Wait for the started transfer to finish.
 *
 * Polls DST_I_STS for DONE or an error (AM026 Table 225), clears it (W1C),
 * then invalidates the buffer cache lines on success.
 *
 * @param dmaDev DMA instance.
 * @param ch     WHAL_VERSAL2_OSPI_DMA_CH.
 *
 * @retval WHAL_SUCCESS   The data is in the buffer.
 * @retval WHAL_EINVAL    Null pointer or bad channel.
 * @retval WHAL_EHARDWARE The DMA reported an error.
 * @retval WHAL_ETIMEOUT  The DMA did not finish in time.
 */
whal_Error whal_Versal2_OspiDma_Wait(whal_Dma *dmaDev, size_t ch);
#endif /* !WHAL_CFG_VERSAL2_OSPI_DMA_DIRECT_API_MAPPING */

/* PMC_IOP_SLCR OSPI_AXI_Sel, bit 1 OSPI_MUX_SEL (AM028): 0 = the OSPI
 * AXI port is driven by the OSPI DMA, 1 = linear (direct mode window). */
#define WHAL_VERSAL2_OSPI_AXI_SEL_ADDR 0xF1060504UL
#define WHAL_VERSAL2_OSPI_MUX_SEL_Msk  (1UL << 1)

/*
 * @brief Switch the OSPI AXI port to the direct mode window (on = 1) or
 * back to the OSPI DMA (on = 0).
 *
 * For the Cadence driver's MapSelect: a map turns it on, closing it turns
 * it off, so DMA reads work again.
 *
 * @param on 1 = window, 0 = DMA.
 */
void whal_Versal2_Ospi_MapSelect(int on);

#endif /* WHAL_VERSAL2_OSPI_DMA_H */
