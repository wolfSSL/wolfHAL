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
#include <wolfHAL/dma/versal2_ospi_dma.h>
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

/* Destination status (AM028 OSPIDMA_DST_STS). BUSY: the channel can't
 * take a new command. DONE_CNT is cleared by writing 3b111. */
#define DMA_DST_STS_REG         0x1808
#define DMA_DST_STS_BUSY_Msk    (1UL << 0)
#define DMA_DST_STS_DONE_CNT_Msk (7UL << 13)

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

#ifdef WHAL_CFG_VERSAL2_OSPI_DMA_DIRECT_API_MAPPING
#define whal_Versal2_OspiDma_Init      whal_Dma_Init
#define whal_Versal2_OspiDma_Deinit    whal_Dma_Deinit
#define whal_Versal2_OspiDma_Configure whal_Dma_Configure
#define whal_Versal2_OspiDma_Start     whal_Dma_Start
#define whal_Versal2_OspiDma_Stop      whal_Dma_Stop
#define whal_Versal2_OspiDma_Wait      whal_Dma_Wait
#endif /* WHAL_CFG_VERSAL2_OSPI_DMA_DIRECT_API_MAPPING */

/* Get the cfg; ch must be the one channel. */
static inline whal_Error whal_Versal2_OspiDma_GetCfg(
    whal_Dma *dmaDev, size_t ch, const whal_Versal2_OspiDma_Cfg **cfg)
{
    if (!dmaDev || !dmaDev->cfg || ch != WHAL_VERSAL2_OSPI_DMA_CH) {
        return WHAL_EINVAL;
    }
    *cfg = (const whal_Versal2_OspiDma_Cfg *)dmaDev->cfg;
    if (!(*cfg)->xfer) {
        return WHAL_EINVAL;
    }
    return WHAL_SUCCESS;
}

whal_Error whal_Versal2_OspiDma_Init(whal_Dma *dmaDev)
{
    const whal_Versal2_OspiDma_Cfg *cfg;
    whal_Error err;

    err = whal_Versal2_OspiDma_GetCfg(dmaDev, WHAL_VERSAL2_OSPI_DMA_CH, &cfg);
    if (err) {
        return err;
    }
    cfg->xfer->buf = NULL;
    cfg->xfer->len = 0;
    return whal_Versal2_OspiDma_Stop(dmaDev, WHAL_VERSAL2_OSPI_DMA_CH);
}

whal_Error whal_Versal2_OspiDma_Deinit(whal_Dma *dmaDev)
{
    if (!dmaDev) {
        return WHAL_EINVAL;
    }
    return WHAL_SUCCESS;
}

whal_Error whal_Versal2_OspiDma_Configure(whal_Dma *dmaDev, size_t ch,
                                          const void *chCfg)
{
    const whal_Versal2_OspiDma_Cfg *cfg;
    const whal_DmaMemXfer *x = chCfg;
    uint64_t dst;
    whal_Error err;

    err = whal_Versal2_OspiDma_GetCfg(dmaDev, ch, &cfg);
    if (err) {
        return err;
    }
    if (!x || !x->buf || x->len == 0) {
        return WHAL_EINVAL;
    }
    if (((uintptr_t)x->buf & DMA_WORD_ALIGN_Msk) != 0 ||
        (x->len & DMA_WORD_ALIGN_Msk) != 0) {
        return WHAL_EINVAL;
    }
    /* A longer length would be cut to the SIZE field (AM028 DST_SIZE). */
    if (x->len > WHAL_VERSAL2_OSPI_DMA_MAX_LEN) {
        return WHAL_EINVAL;
    }
    /* A busy channel can't take a new command (AM028 DST_STS BUSY). */
    if (whal_Reg32_Read(dmaDev->base, DMA_DST_STS_REG) &
        DMA_DST_STS_BUSY_Msk) {
        return WHAL_ENOTREADY;
    }
    /* Clear old status (W1C): a DONE left by a transfer that ended after
     * its Wait gave up would end the next Wait at once. */
    whal_Reg32_Write(dmaDev->base, DMA_DST_I_STS_REG,
                     whal_Reg32_Read(dmaDev->base, DMA_DST_I_STS_REG));

    *cfg->xfer = *x;
    dst = (uint64_t)(uintptr_t)x->buf;

    /* Flush the buffer so old cache lines can't overwrite the DMA data. */
    if (cfg->CacheCleanInval) {
        cfg->CacheCleanInval(x->buf, x->len);
    }

    /* Where the data goes in memory (AM026 p707 step 2a). */
    whal_Reg32_Write(dmaDev->base, DMA_DST_ADDR_REG, (uint32_t)dst);
    whal_Reg32_Write(dmaDev->base, DMA_DST_ADDR_MSB_REG,
                     (uint32_t)(dst >> 32));

    /* Read from the indirect trigger window (AM026 p707 step 2b). */
    whal_Reg32_Write(dmaDev->base, DMA_SRC_RD_ADDR_REG, cfg->srcRdAddr);

    /* DST control (AM026 p707 step 2d). */
    whal_Reg32_Write(dmaDev->base, DMA_DST_CTRL_REG, cfg->dstCtrl);

    return WHAL_SUCCESS;
}

whal_Error whal_Versal2_OspiDma_Start(whal_Dma *dmaDev, size_t ch)
{
    const whal_Versal2_OspiDma_Cfg *cfg;
    whal_Error err;

    err = whal_Versal2_OspiDma_GetCfg(dmaDev, ch, &cfg);
    if (err) {
        return err;
    }
    if (cfg->xfer->len == 0) {
        return WHAL_EINVAL;
    }
    /* A running transfer can't be started again (AM028 DST_STS BUSY). */
    if (whal_Reg32_Read(dmaDev->base, DMA_DST_STS_REG) &
        DMA_DST_STS_BUSY_Msk) {
        return WHAL_ENOTREADY;
    }

    /* DST_SIZE starts the DMA. It is in bytes: the AM028 SIZE field is
     * bits 28:2, so bytes there are words (AM026 p707). */
    whal_Reg32_Write(dmaDev->base, DMA_DST_SIZE_REG,
                     (uint32_t)cfg->xfer->len);
    return WHAL_SUCCESS;
}

whal_Error whal_Versal2_OspiDma_Stop(whal_Dma *dmaDev, size_t ch)
{
    const whal_Versal2_OspiDma_Cfg *cfg;
    whal_Error err;

    err = whal_Versal2_OspiDma_GetCfg(dmaDev, ch, &cfg);
    if (err) {
        return err;
    }
    /* Clear the status bits (W1C) and the finished-transfer count. */
    whal_Reg32_Write(dmaDev->base, DMA_DST_I_STS_REG,
                     whal_Reg32_Read(dmaDev->base, DMA_DST_I_STS_REG));
    whal_Reg32_Write(dmaDev->base, DMA_DST_STS_REG,
                     DMA_DST_STS_DONE_CNT_Msk);
    /* AM026/AM028 give no abort: a running transfer must finish. */
    if (whal_Reg32_Read(dmaDev->base, DMA_DST_STS_REG) &
        DMA_DST_STS_BUSY_Msk) {
        return WHAL_ENOTREADY;
    }
    return WHAL_SUCCESS;
}

whal_Error whal_Versal2_OspiDma_Wait(whal_Dma *dmaDev, size_t ch)
{
    const whal_Versal2_OspiDma_Cfg *cfg;
    uint32_t sts;
    whal_Error err;

    err = whal_Versal2_OspiDma_GetCfg(dmaDev, ch, &cfg);
    if (err) {
        return err;
    }
    /* Nothing configured: there is no transfer and no buffer to wait for. */
    if (cfg->xfer->len == 0) {
        return WHAL_EINVAL;
    }

    WHAL_TIMEOUT_START(cfg->timeout);
    for (;;) {
        sts = whal_Reg32_Read(dmaDev->base, DMA_DST_I_STS_REG);

        /* Check errors first: DONE with an error bit is still a failure. */
        if ((sts & DMA_DST_I_STS_ERR_Msk) != 0) {
            /* Clear the bits (W1C) so the next read starts clean. */
            whal_Reg32_Write(dmaDev->base, DMA_DST_I_STS_REG, sts);
            return WHAL_EHARDWARE;
        }
        if ((sts & DMA_DST_I_STS_DONE_Msk) != 0) {
            break;
        }
        if (WHAL_TIMEOUT_EXPIRED(cfg->timeout)) {
            return WHAL_ETIMEOUT;
        }
    }

    /* Clear DONE (W1C). */
    whal_Reg32_Write(dmaDev->base, DMA_DST_I_STS_REG, sts);

    /* Drop stale cache lines so the CPU reads the new DMA data. */
    if (cfg->CacheInval) {
        cfg->CacheInval(cfg->xfer->buf, cfg->xfer->len);
    }

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_VERSAL2_OSPI_DMA_DIRECT_API_MAPPING
const whal_DmaDriver whal_Versal2_OspiDma_Driver = {
    .Init = whal_Versal2_OspiDma_Init,
    .Deinit = whal_Versal2_OspiDma_Deinit,
    .Configure = whal_Versal2_OspiDma_Configure,
    .Start = whal_Versal2_OspiDma_Start,
    .Stop = whal_Versal2_OspiDma_Stop,
    .Wait = whal_Versal2_OspiDma_Wait,
};
#endif /* !WHAL_CFG_VERSAL2_OSPI_DMA_DIRECT_API_MAPPING */

void whal_Versal2_Ospi_MapSelect(int on)
{
    uint32_t sel = 0;

    if (on) {
        sel = WHAL_VERSAL2_OSPI_MUX_SEL_Msk;
    }
    whal_Reg32_Update(WHAL_VERSAL2_OSPI_AXI_SEL_ADDR, 0,
                      WHAL_VERSAL2_OSPI_MUX_SEL_Msk, sel);
}
