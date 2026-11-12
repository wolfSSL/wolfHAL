/* versal2_ospi_dma.c
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

#include <stddef.h>
#include <stdint.h>
#include <wolfHAL/spimem/versal2_ospi_dma.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/timeout.h>

/* OSPI DMA registers, offsets from the OSPI base (AM028 OSPIDMA_*):
 * source DMA at +0x1000, destination DMA at +0x1800. */

/* Source read address (AM028 OSPIDMA_SRC_RD_ADDR) */
#define DMA_SRC_RD_ADDR_REG     0x1000

/* Destination address, bits 31:0; bits 1:0 read as 0 (AM028) */
#define DMA_DST_ADDR_REG        0x1800

/* Destination size; writing it starts the DMA (AM028 OSPIDMA_DST_SIZE) */
#define DMA_DST_SIZE_REG        0x1804

/* Destination control (AM028 OSPIDMA_DST_CTRL) */
#define DMA_DST_CTRL_REG        0x180C

/* Destination interrupt status, write 1 to clear (AM028, AM026 Table 225) */
#define DMA_DST_I_STS_REG       0x1814

/* Destination address, bits 48:32 (AM028 OSPIDMA_DST_ADDR_MSB) */
#define DMA_DST_ADDR_MSB_REG    0x1828

/* DST_I_STS bits (AM026 Table 225 p708-709, AM028 OSPIDMA_DST_I_STS) */
#define DMA_DST_I_STS_DONE_Pos          1
#define DMA_DST_I_STS_DONE_Msk          (1UL << DMA_DST_I_STS_DONE_Pos)

#define DMA_DST_I_STS_AXI_BRESP_ERR_Pos 2
#define DMA_DST_I_STS_AXI_BRESP_ERR_Msk (1UL << DMA_DST_I_STS_AXI_BRESP_ERR_Pos)

#define DMA_DST_I_STS_TIMEOUT_STRM_Pos  3
#define DMA_DST_I_STS_TIMEOUT_STRM_Msk  (1UL << DMA_DST_I_STS_TIMEOUT_STRM_Pos)

#define DMA_DST_I_STS_TIMEOUT_MEM_Pos   4
#define DMA_DST_I_STS_TIMEOUT_MEM_Msk   (1UL << DMA_DST_I_STS_TIMEOUT_MEM_Pos)

#define DMA_DST_I_STS_FIFO_OVERFLOW_Pos 7
#define DMA_DST_I_STS_FIFO_OVERFLOW_Msk (1UL << DMA_DST_I_STS_FIFO_OVERFLOW_Pos)

/* Any of these bits fails the read. */
#define DMA_DST_I_STS_ERR_Msk  (DMA_DST_I_STS_AXI_BRESP_ERR_Msk | \
                                DMA_DST_I_STS_TIMEOUT_STRM_Msk  | \
                                DMA_DST_I_STS_TIMEOUT_MEM_Msk   | \
                                DMA_DST_I_STS_FIFO_OVERFLOW_Msk)

/* Address and size must be 4-byte aligned (AM026 p706). */
#define DMA_WORD_ALIGN_Msk      0x3UL

whal_Error whal_Versal2_OspiDma_Arm(void *ctx, void *buf, size_t len)
{
    whal_Versal2_OspiDma_Ctx *dma;
    uint64_t dst;

    if (!ctx || !buf || len == 0) {
        return WHAL_EINVAL;
    }
    if (((uintptr_t)buf & DMA_WORD_ALIGN_Msk) != 0 ||
        (len & DMA_WORD_ALIGN_Msk) != 0) {
        return WHAL_EINVAL;
    }

    dma = (whal_Versal2_OspiDma_Ctx *)ctx;
    dst = (uint64_t)(uintptr_t)buf;

    /* Flush the buffer so old cache lines can't overwrite the DMA data. */
    if (dma->CacheCleanInval) {
        dma->CacheCleanInval(buf, len);
    }

    /* Where the data goes in memory (AM026 p707 step 2a). */
    whal_Reg32_Write(dma->base, DMA_DST_ADDR_REG, (uint32_t)dst);
    whal_Reg32_Write(dma->base, DMA_DST_ADDR_MSB_REG, (uint32_t)(dst >> 32));

    /* Read from the indirect trigger window (AM026 p707 step 2b). */
    whal_Reg32_Write(dma->base, DMA_SRC_RD_ADDR_REG, dma->srcRdAddr);

    /* DST control (AM026 p707 step 2d). */
    whal_Reg32_Write(dma->base, DMA_DST_CTRL_REG, dma->dstCtrl);

    /* DST_SIZE starts the DMA, so last. It is in bytes: the AM028 SIZE
     * field is bits 28:2, so bytes there are words (AM026 p707). */
    whal_Reg32_Write(dma->base, DMA_DST_SIZE_REG, (uint32_t)len);

    return WHAL_SUCCESS;
}

whal_Error whal_Versal2_OspiDma_Wait(void *ctx, void *buf, size_t len)
{
    whal_Versal2_OspiDma_Ctx *dma;
    uint32_t sts;

    if (!ctx) {
        return WHAL_EINVAL;
    }

    dma = (whal_Versal2_OspiDma_Ctx *)ctx;

    WHAL_TIMEOUT_START(dma->timeout);
    for (;;) {
        sts = whal_Reg32_Read(dma->base, DMA_DST_I_STS_REG);

        /* Check errors first: DONE with an error bit is still a failure. */
        if ((sts & DMA_DST_I_STS_ERR_Msk) != 0) {
            /* Clear the bits (W1C) so the next read starts clean. */
            whal_Reg32_Write(dma->base, DMA_DST_I_STS_REG, sts);
            return WHAL_EHARDWARE;
        }
        if ((sts & DMA_DST_I_STS_DONE_Msk) != 0) {
            break;
        }
        if (WHAL_TIMEOUT_EXPIRED(dma->timeout)) {
            return WHAL_ETIMEOUT;
        }
    }

    /* Clear DONE (W1C). */
    whal_Reg32_Write(dma->base, DMA_DST_I_STS_REG, sts);

    /* Drop stale cache lines so the CPU reads the new DMA data. */
    if (dma->CacheInval) {
        dma->CacheInval(buf, len);
    }

    return WHAL_SUCCESS;
}
