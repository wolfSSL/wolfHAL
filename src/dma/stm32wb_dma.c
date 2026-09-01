/* stm32wb_dma.c
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

#include <stdint.h>
#ifdef WHAL_CFG_STM32WB_DMA_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_Stm32wb_Dma_Dev singleton */
#endif
#include <wolfHAL/dma/dma.h>
#include <wolfHAL/dma/stm32wb_dma.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * STM32WB DMA Register Definitions
 *
 * DMA1 has 7 channels, DMA2 has 5 channels. Each channel has a set of
 * registers at offset 0x08 + 0x14 * (ch - 1) from the DMA base, where
 * ch is the 1-based hardware channel number.
 */

/* DMA interrupt status register (read-only) */
#define DMA_ISR_REG             0x00

/* DMA interrupt flag clear register (write-only) */
#define DMA_IFCR_REG            0x04

/* Per-channel registers. hw_ch is 1-based hardware channel number. */
#define DMA_CCR_REG(hw_ch)      (0x08 + 0x14 * ((hw_ch) - 1))
#define DMA_CNDTR_REG(hw_ch)    (0x0C + 0x14 * ((hw_ch) - 1))
#define DMA_CPAR_REG(hw_ch)     (0x10 + 0x14 * ((hw_ch) - 1))
#define DMA_CMAR_REG(hw_ch)     (0x14 + 0x14 * ((hw_ch) - 1))

/* CCR bit definitions */
#define DMA_CCR_EN_Pos          0
#define DMA_CCR_EN_Msk          (1UL << DMA_CCR_EN_Pos)

#define DMA_CCR_TCIE_Pos        1
#define DMA_CCR_TCIE_Msk        (1UL << DMA_CCR_TCIE_Pos)

#define DMA_CCR_HTIE_Pos        2
#define DMA_CCR_HTIE_Msk        (1UL << DMA_CCR_HTIE_Pos)

#define DMA_CCR_TEIE_Pos        3
#define DMA_CCR_TEIE_Msk        (1UL << DMA_CCR_TEIE_Pos)

#define DMA_CCR_DIR_Pos         4
#define DMA_CCR_DIR_Msk         (1UL << DMA_CCR_DIR_Pos)

#define DMA_CCR_CIRC_Pos        5
#define DMA_CCR_CIRC_Msk        (1UL << DMA_CCR_CIRC_Pos)

#define DMA_CCR_PINC_Pos        6
#define DMA_CCR_PINC_Msk        (1UL << DMA_CCR_PINC_Pos)

#define DMA_CCR_MINC_Pos        7
#define DMA_CCR_MINC_Msk        (1UL << DMA_CCR_MINC_Pos)

#define DMA_CCR_PSIZE_Pos       8
#define DMA_CCR_PSIZE_Msk       (WHAL_BITMASK(2) << DMA_CCR_PSIZE_Pos)

#define DMA_CCR_MSIZE_Pos       10
#define DMA_CCR_MSIZE_Msk       (WHAL_BITMASK(2) << DMA_CCR_MSIZE_Pos)

#define DMA_CCR_PL_Pos          12
#define DMA_CCR_PL_Msk          (WHAL_BITMASK(2) << DMA_CCR_PL_Pos)

#define DMA_CCR_MEM2MEM_Pos     14
#define DMA_CCR_MEM2MEM_Msk     (1UL << DMA_CCR_MEM2MEM_Pos)

/*
 * ISR/IFCR flag positions. Each channel uses 4 bits:
 *   bit 0: GIF  (global interrupt flag)
 *   bit 1: TCIF (transfer complete)
 *   bit 2: HTIF (half transfer)
 *   bit 3: TEIF (transfer error)
 * Channel 1 starts at bit 0, channel 2 at bit 4, etc.
 */
#define DMA_ISR_GIF_Pos(hw_ch)  (((hw_ch) - 1) * 4)
#define DMA_ISR_TCIF_Pos(hw_ch) (((hw_ch) - 1) * 4 + 1)
#define DMA_ISR_TEIF_Pos(hw_ch) (((hw_ch) - 1) * 4 + 3)

#define DMA_IFCR_CGIF(hw_ch)    (1UL << (((hw_ch) - 1) * 4))
#define DMA_IFCR_CTCIF(hw_ch)   (1UL << (((hw_ch) - 1) * 4 + 1))
#define DMA_IFCR_CHTIF(hw_ch)   (1UL << (((hw_ch) - 1) * 4 + 2))
#define DMA_IFCR_CTEIF(hw_ch)   (1UL << (((hw_ch) - 1) * 4 + 3))

/* Clear all flags for a given hardware channel */
#define DMA_IFCR_CALL(hw_ch)    (0xFUL << (((hw_ch) - 1) * 4))

/* DMAMUX channel configuration register */
#define DMAMUX_CxCR_REG(mux_ch)  (0x000 + 0x04 * (mux_ch))

#define DMAMUX_CxCR_DMAREQ_ID_Pos  0
#define DMAMUX_CxCR_DMAREQ_ID_Msk  (WHAL_BITMASK(6) << DMAMUX_CxCR_DMAREQ_ID_Pos)

#ifdef WHAL_CFG_STM32WB_DMA_DIRECT_API_MAPPING
#define whal_Stm32wb_Dma_Init      whal_Dma_Init
#define whal_Stm32wb_Dma_Deinit    whal_Dma_Deinit
#define whal_Stm32wb_Dma_Configure whal_Dma_Configure
#define whal_Stm32wb_Dma_Start     whal_Dma_Start
#define whal_Stm32wb_Dma_Stop      whal_Dma_Stop
#endif /* WHAL_CFG_STM32WB_DMA_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_STM32WB_DMA_SINGLE_INSTANCE
const whal_Dma whal_Stm32wb_Dma_Dev = WHAL_CFG_STM32WB_DMA_DEV;
#endif

whal_Error whal_Stm32wb_Dma_Init(whal_Dma *dmaDev)
{
#ifdef WHAL_CFG_STM32WB_DMA_SINGLE_INSTANCE
    whal_Stm32wb_Dma_Cfg *cfg =
        (whal_Stm32wb_Dma_Cfg *)whal_Stm32wb_Dma_Dev.cfg;
    size_t base = whal_Stm32wb_Dma_Dev.base;
    (void)dmaDev;
#else
    whal_Stm32wb_Dma_Cfg *cfg;
    size_t base;

    if (!dmaDev || !dmaDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wb_Dma_Cfg *)dmaDev->cfg;
    base = dmaDev->base;
#endif

    /* Clear all interrupt flags for all channels */
    {
        size_t clearAll = 0;
        for (size_t i = 1; i <= cfg->numChannels; ++i) {
            clearAll |= DMA_IFCR_CALL(i);
        }
        whal_Reg_Write(base, DMA_IFCR_REG, clearAll);
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Dma_Deinit(whal_Dma *dmaDev)
{
#ifdef WHAL_CFG_STM32WB_DMA_SINGLE_INSTANCE
    whal_Stm32wb_Dma_Cfg *cfg =
        (whal_Stm32wb_Dma_Cfg *)whal_Stm32wb_Dma_Dev.cfg;
    size_t base = whal_Stm32wb_Dma_Dev.base;
    (void)dmaDev;
#else
    whal_Stm32wb_Dma_Cfg *cfg;
    size_t base;

    if (!dmaDev || !dmaDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wb_Dma_Cfg *)dmaDev->cfg;
    base = dmaDev->base;
#endif

    /* Disable all channels and clear all interrupt flags */
    for (size_t i = 1; i <= cfg->numChannels; ++i) {
        whal_Reg_Update(base, DMA_CCR_REG(i),
                        DMA_CCR_EN_Msk,
                        whal_SetBits(DMA_CCR_EN_Msk, DMA_CCR_EN_Pos, 0));
    }

    {
        size_t clearAll = 0;
        for (size_t i = 1; i <= cfg->numChannels; ++i) {
            clearAll |= DMA_IFCR_CALL(i);
        }
        whal_Reg_Write(base, DMA_IFCR_REG, clearAll);
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Dma_Configure(whal_Dma *dmaDev, size_t ch,
                                             const void *chCfg)
{
    const whal_Stm32wb_Dma_ChCfg *dmaChCfg;
    size_t hw_ch;
    size_t ccr;
    uint32_t periphAddr;
    uint32_t memAddr;
    size_t periphInc;
    size_t memInc;
    size_t periphSize;
    size_t memSize;
#ifdef WHAL_CFG_STM32WB_DMA_SINGLE_INSTANCE
    whal_Stm32wb_Dma_Cfg *cfg =
        (whal_Stm32wb_Dma_Cfg *)whal_Stm32wb_Dma_Dev.cfg;
    size_t base = whal_Stm32wb_Dma_Dev.base;
    (void)dmaDev;

    if (!chCfg) {
        return WHAL_EINVAL;
    }
#else
    whal_Stm32wb_Dma_Cfg *cfg;
    size_t base;

    if (!dmaDev || !dmaDev->cfg || !chCfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wb_Dma_Cfg *)dmaDev->cfg;
    base = dmaDev->base;
#endif
    dmaChCfg = (const whal_Stm32wb_Dma_ChCfg *)chCfg;

    if (ch >= cfg->numChannels) {
        return WHAL_EINVAL;
    }

    /* Hardware channels are 1-based */
    hw_ch = ch + 1;

    /*
     * Map the direction, src/dst addresses, and increment modes to the
     * hardware's CPAR/CMAR/DIR/PINC/MINC/PSIZE/MSIZE model.
     *
     * DIR=0: peripheral (CPAR) -> memory (CMAR)  [read from peripheral]
     * DIR=1: memory (CMAR) -> peripheral (CPAR)  [read from memory]
     * MEM2MEM=1: memory (CPAR with DIR=0 is src) -> memory (CMAR is dst)
     */
    switch (dmaChCfg->dir) {
    case WHAL_STM32WB_DMA_DIR_PERIPH_TO_MEM:
        periphAddr = dmaChCfg->srcAddr;
        memAddr = dmaChCfg->dstAddr;
        periphInc = dmaChCfg->srcInc;
        memInc = dmaChCfg->dstInc;
        periphSize = dmaChCfg->width;
        memSize = dmaChCfg->width;
        break;
    case WHAL_STM32WB_DMA_DIR_MEM_TO_PERIPH:
        periphAddr = dmaChCfg->dstAddr;
        memAddr = dmaChCfg->srcAddr;
        periphInc = dmaChCfg->dstInc;
        memInc = dmaChCfg->srcInc;
        periphSize = dmaChCfg->width;
        memSize = dmaChCfg->width;
        break;
    case WHAL_STM32WB_DMA_DIR_MEM_TO_MEM:
        /* MEM2MEM: CPAR=source, CMAR=destination, DIR=0 */
        periphAddr = dmaChCfg->srcAddr;
        memAddr = dmaChCfg->dstAddr;
        periphInc = dmaChCfg->srcInc;
        memInc = dmaChCfg->dstInc;
        periphSize = dmaChCfg->width;
        memSize = dmaChCfg->width;
        break;
    default:
        return WHAL_EINVAL;
    }

    /* Build the CCR value */
    ccr = 0;

    /* DIR bit: 0 for periph-to-mem and mem-to-mem, 1 for mem-to-periph */
    if (dmaChCfg->dir == WHAL_STM32WB_DMA_DIR_MEM_TO_PERIPH) {
        ccr |= whal_SetBits(DMA_CCR_DIR_Msk, DMA_CCR_DIR_Pos, 1);
    }

    /* MEM2MEM bit */
    if (dmaChCfg->dir == WHAL_STM32WB_DMA_DIR_MEM_TO_MEM) {
        ccr |= whal_SetBits(DMA_CCR_MEM2MEM_Msk, DMA_CCR_MEM2MEM_Pos, 1);
    }

    /* Circular mode */
    if (dmaChCfg->circular) {
        ccr |= whal_SetBits(DMA_CCR_CIRC_Msk, DMA_CCR_CIRC_Pos, 1);
    }

    /* Peripheral increment */
    ccr |= whal_SetBits(DMA_CCR_PINC_Msk, DMA_CCR_PINC_Pos, periphInc);

    /* Memory increment */
    ccr |= whal_SetBits(DMA_CCR_MINC_Msk, DMA_CCR_MINC_Pos, memInc);

    /* Peripheral size */
    ccr |= whal_SetBits(DMA_CCR_PSIZE_Msk, DMA_CCR_PSIZE_Pos, periphSize);

    /* Memory size */
    ccr |= whal_SetBits(DMA_CCR_MSIZE_Msk, DMA_CCR_MSIZE_Pos, memSize);

    /* Enable transfer complete and transfer error interrupts */
    ccr |= whal_SetBits(DMA_CCR_TCIE_Msk, DMA_CCR_TCIE_Pos, 1);
    ccr |= whal_SetBits(DMA_CCR_TEIE_Msk, DMA_CCR_TEIE_Pos, 1);

    /* Write channel registers. EN must be 0 before writing CCR fields. */
    whal_Reg_Write(base, DMA_CCR_REG(hw_ch), 0);
    whal_Reg_Write(base, DMA_CNDTR_REG(hw_ch), dmaChCfg->length);
    whal_Reg_Write(base, DMA_CPAR_REG(hw_ch), periphAddr);
    whal_Reg_Write(base, DMA_CMAR_REG(hw_ch), memAddr);
    whal_Reg_Write(base, DMA_CCR_REG(hw_ch), ccr);

    /* Configure DMAMUX request mapping */
    size_t muxCh = cfg->dmamuxChOffset + ch;
    whal_Reg_Update(cfg->dmamuxBase, DMAMUX_CxCR_REG(muxCh),
                    DMAMUX_CxCR_DMAREQ_ID_Msk,
                    whal_SetBits(DMAMUX_CxCR_DMAREQ_ID_Msk,
                                 DMAMUX_CxCR_DMAREQ_ID_Pos,
                                 dmaChCfg->dmamuxReqId));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Dma_Start(whal_Dma *dmaDev, size_t ch)
{
    size_t hw_ch;
#ifdef WHAL_CFG_STM32WB_DMA_SINGLE_INSTANCE
    whal_Stm32wb_Dma_Cfg *cfg =
        (whal_Stm32wb_Dma_Cfg *)whal_Stm32wb_Dma_Dev.cfg;
    size_t base = whal_Stm32wb_Dma_Dev.base;
    (void)dmaDev;
#else
    whal_Stm32wb_Dma_Cfg *cfg;
    size_t base;

    if (!dmaDev || !dmaDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wb_Dma_Cfg *)dmaDev->cfg;
    base = dmaDev->base;
#endif

    if (ch >= cfg->numChannels) {
        return WHAL_EINVAL;
    }

    hw_ch = ch + 1;

    /* Clear stale interrupt flags before enabling (TRM requires this) */
    whal_Reg_Write(base, DMA_IFCR_REG, DMA_IFCR_CALL(hw_ch));

    /* Enable the channel */
    whal_Reg_Update(base, DMA_CCR_REG(hw_ch),
                    DMA_CCR_EN_Msk,
                    whal_SetBits(DMA_CCR_EN_Msk, DMA_CCR_EN_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Dma_Stop(whal_Dma *dmaDev, size_t ch)
{
    size_t hw_ch;
#ifdef WHAL_CFG_STM32WB_DMA_SINGLE_INSTANCE
    whal_Stm32wb_Dma_Cfg *cfg =
        (whal_Stm32wb_Dma_Cfg *)whal_Stm32wb_Dma_Dev.cfg;
    size_t base = whal_Stm32wb_Dma_Dev.base;
    (void)dmaDev;
#else
    whal_Stm32wb_Dma_Cfg *cfg;
    size_t base;

    if (!dmaDev || !dmaDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wb_Dma_Cfg *)dmaDev->cfg;
    base = dmaDev->base;
#endif

    if (ch >= cfg->numChannels) {
        return WHAL_EINVAL;
    }

    hw_ch = ch + 1;

    /* Disable the channel */
    whal_Reg_Update(base, DMA_CCR_REG(hw_ch),
                    DMA_CCR_EN_Msk,
                    whal_SetBits(DMA_CCR_EN_Msk, DMA_CCR_EN_Pos, 0));

    /* Clear channel interrupt flags */
    whal_Reg_Write(base, DMA_IFCR_REG, DMA_IFCR_CALL(hw_ch));

    return WHAL_SUCCESS;
}

void whal_Stm32wb_Dma_IRQHandler(whal_Dma *dmaDev, size_t ch,
                                 whal_Stm32wb_Dma_Callback cb, void *ctx)
{
    size_t hw_ch;
    size_t isr;
#ifdef WHAL_CFG_STM32WB_DMA_SINGLE_INSTANCE
    size_t base = whal_Stm32wb_Dma_Dev.base;
    (void)dmaDev;
#else
    size_t base;

    if (!dmaDev)
        return;

    base = dmaDev->base;
#endif

    hw_ch = ch + 1;

    isr = whal_Reg_Read(base, DMA_ISR_REG);

    if (isr & (1UL << DMA_ISR_TCIF_Pos(hw_ch))) {
        whal_Reg_Write(base, DMA_IFCR_REG, DMA_IFCR_CTCIF(hw_ch));
        if (cb)
            cb(ctx, WHAL_SUCCESS);
    }

    if (isr & (1UL << DMA_ISR_TEIF_Pos(hw_ch))) {
        whal_Reg_Write(base, DMA_IFCR_REG, DMA_IFCR_CTEIF(hw_ch));
        if (cb)
            cb(ctx, WHAL_EHARDWARE);
    }
}

#ifndef WHAL_CFG_STM32WB_DMA_DIRECT_API_MAPPING
const whal_DmaDriver whal_Stm32wb_Dma_Driver = {
    .Init = whal_Stm32wb_Dma_Init,
    .Deinit = whal_Stm32wb_Dma_Deinit,
    .Configure = whal_Stm32wb_Dma_Configure,
    .Start = whal_Stm32wb_Dma_Start,
    .Stop = whal_Stm32wb_Dma_Stop,
};
#endif /* !WHAL_CFG_STM32WB_DMA_DIRECT_API_MAPPING */
