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

/*
 * @file versal2_ospi_dma.h
 * @brief AMD Versal Gen 2 OSPI DMA read hook.
 *
 * Arm/Wait for the Cadence driver's DMA hook (AM026 p706-709); not a whal_Dma
 * driver. Needs 4-byte alignment; set the hook align to the cache line.
 */

/* Source read address = IND_AHB_ADDR_TRIG (AM026 p707 step 2b), the flash
 * window start (p704). */
#define WHAL_VERSAL2_OSPI_DMA_SRC_RD_ADDR 0xC0000000UL

/* DST_CTRL: reset value 0x803FFA00 (AM028) with SSS_FIFOTHRESH (31:25) at
 * its max 0x7A, for the AM026 Table 223 settings (p707-708). */
#define WHAL_VERSAL2_OSPI_DMA_DST_CTRL    0xF43FFA00UL

/*
 * @brief DMA hook context. Pass a pointer to it as the hook's ctx.
 */
typedef struct {
    uintptr_t base;          /* OSPI base; DMA blocks at +0x1000/+0x1800 */
    uint32_t srcRdAddr;      /* DMA_SRC_RD_ADDR (AMD: 0xC0000000) */
    uint32_t dstCtrl;        /* DMA_DST_CTRL (AMD: 0xF43FFA00) */
    void (*CacheCleanInval)(void *buf, size_t len);   /* before (optional) */
    void (*CacheInval)(void *buf, size_t len);        /* after (optional) */
    whal_Timeout *timeout;   /* bound for Wait; NULL = wait forever */
} whal_Versal2_OspiDma_Ctx;

/*
 * @brief Arm the destination DMA for one read into a buffer.
 *
 * Cleans the buffer cache lines, then programs the DMA. The DST_SIZE write
 * goes last and starts it (AM026 p707).
 *
 * @param ctx Pointer to a whal_Versal2_OspiDma_Ctx.
 * @param buf Destination buffer, 4-byte aligned.
 * @param len Byte count, not 0, a multiple of 4.
 *
 * @retval WHAL_SUCCESS The DMA is armed.
 * @retval WHAL_EINVAL  NULL ctx or buf, len 0, or buf or len not 4-byte
 *                      aligned.
 */
whal_Error whal_Versal2_OspiDma_Arm(void *ctx, void *buf, size_t len);

/*
 * @brief Wait for the DMA armed by whal_Versal2_OspiDma_Arm to finish.
 *
 * Polls DST_I_STS for DONE or an error (AM026 Table 225), clears it (W1C),
 * then invalidates the buffer cache lines on success.
 *
 * @param ctx Pointer to a whal_Versal2_OspiDma_Ctx.
 * @param buf The buffer passed to Arm.
 * @param len The length passed to Arm.
 *
 * @retval WHAL_SUCCESS   The data is in the buffer.
 * @retval WHAL_EINVAL    NULL ctx.
 * @retval WHAL_EHARDWARE The DMA reported an error.
 * @retval WHAL_ETIMEOUT  The DMA did not finish in time.
 */
whal_Error whal_Versal2_OspiDma_Wait(void *ctx, void *buf, size_t len);

#endif /* WHAL_VERSAL2_OSPI_DMA_H */
