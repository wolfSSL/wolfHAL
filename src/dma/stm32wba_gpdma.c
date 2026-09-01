/* stm32wba_gpdma.c
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
#include <stddef.h>
#ifdef WHAL_CFG_STM32WBA_GPDMA_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_Stm32wba_Gpdma_Dev singleton (possibly via platform alias macro) */
#endif
#include <wolfHAL/dma/stm32wba_gpdma.h>
#include <wolfHAL/dma/dma.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

/*
 * STM32WBA GPDMA Register Definitions (RM0493 chapter 17)
 *
 * Base: 0x40020000
 * Each channel x (0-7) occupies 0x80 bytes starting at 0x50 + 0x80*x.
 */

/* Global registers */
#define GPDMA_SECCFGR_REG    0x00
#define GPDMA_PRIVCFGR_REG   0x04
#define GPDMA_RCFGLOCKR_REG  0x08
#define GPDMA_MISR_REG       0x0C
#define GPDMA_SMISR_REG      0x10

/* Per-channel register offsets (add 0x50 + 0x80*ch) */
#define GPDMA_CH_BASE(ch)    (0x50 + ((ch) * 0x80))

#define GPDMA_CxLBAR(ch)     (GPDMA_CH_BASE(ch) + 0x00)
#define GPDMA_CxFCR(ch)      (GPDMA_CH_BASE(ch) + 0x0C)
#define GPDMA_CxSR(ch)       (GPDMA_CH_BASE(ch) + 0x10)
#define GPDMA_CxCR(ch)       (GPDMA_CH_BASE(ch) + 0x14)
#define GPDMA_CxTR1(ch)      (GPDMA_CH_BASE(ch) + 0x40)
#define GPDMA_CxTR2(ch)      (GPDMA_CH_BASE(ch) + 0x44)
#define GPDMA_CxBR1(ch)      (GPDMA_CH_BASE(ch) + 0x48)
#define GPDMA_CxSAR(ch)      (GPDMA_CH_BASE(ch) + 0x4C)
#define GPDMA_CxDAR(ch)      (GPDMA_CH_BASE(ch) + 0x50)
#define GPDMA_CxLLR(ch)      (GPDMA_CH_BASE(ch) + 0x7C)

/* CxSR bits */
#define GPDMA_CxSR_IDLEF_Pos  0
#define GPDMA_CxSR_IDLEF_Msk  (1UL << GPDMA_CxSR_IDLEF_Pos)
#define GPDMA_CxSR_TCF_Pos    8
#define GPDMA_CxSR_TCF_Msk    (1UL << GPDMA_CxSR_TCF_Pos)
#define GPDMA_CxSR_HTF_Pos    9
#define GPDMA_CxSR_HTF_Msk    (1UL << GPDMA_CxSR_HTF_Pos)
#define GPDMA_CxSR_DTEF_Pos   10
#define GPDMA_CxSR_DTEF_Msk   (1UL << GPDMA_CxSR_DTEF_Pos)
#define GPDMA_CxSR_ULEF_Pos   11
#define GPDMA_CxSR_ULEF_Msk   (1UL << GPDMA_CxSR_ULEF_Pos)
#define GPDMA_CxSR_USEF_Pos   12
#define GPDMA_CxSR_USEF_Msk   (1UL << GPDMA_CxSR_USEF_Pos)
#define GPDMA_CxSR_SUSPF_Pos  13
#define GPDMA_CxSR_SUSPF_Msk  (1UL << GPDMA_CxSR_SUSPF_Pos)
#define GPDMA_CxSR_TOF_Pos    14
#define GPDMA_CxSR_TOF_Msk    (1UL << GPDMA_CxSR_TOF_Pos)

#define GPDMA_CxSR_ALL_ERR (GPDMA_CxSR_DTEF_Msk | GPDMA_CxSR_ULEF_Msk | \
                            GPDMA_CxSR_USEF_Msk)

/* CxFCR bits (write 1 to clear) -- same positions as CxSR flags */
#define GPDMA_CxFCR_ALL (GPDMA_CxSR_TCF_Msk | GPDMA_CxSR_HTF_Msk | \
                         GPDMA_CxSR_DTEF_Msk | GPDMA_CxSR_ULEF_Msk | \
                         GPDMA_CxSR_USEF_Msk | GPDMA_CxSR_SUSPF_Msk | \
                         GPDMA_CxSR_TOF_Msk)

/* CxCR bits */
#define GPDMA_CxCR_EN_Pos     0
#define GPDMA_CxCR_EN_Msk     (1UL << GPDMA_CxCR_EN_Pos)
#define GPDMA_CxCR_RESET_Pos  1
#define GPDMA_CxCR_RESET_Msk  (1UL << GPDMA_CxCR_RESET_Pos)
#define GPDMA_CxCR_SUSP_Pos   2
#define GPDMA_CxCR_SUSP_Msk   (1UL << GPDMA_CxCR_SUSP_Pos)
#define GPDMA_CxCR_TCIE_Pos   8
#define GPDMA_CxCR_TCIE_Msk   (1UL << GPDMA_CxCR_TCIE_Pos)
#define GPDMA_CxCR_HTIE_Pos   9
#define GPDMA_CxCR_HTIE_Msk   (1UL << GPDMA_CxCR_HTIE_Pos)
#define GPDMA_CxCR_DTEIE_Pos  10
#define GPDMA_CxCR_DTEIE_Msk  (1UL << GPDMA_CxCR_DTEIE_Pos)
#define GPDMA_CxCR_ULEIE_Pos  11
#define GPDMA_CxCR_ULEIE_Msk  (1UL << GPDMA_CxCR_ULEIE_Pos)
#define GPDMA_CxCR_USEIE_Pos  12
#define GPDMA_CxCR_USEIE_Msk  (1UL << GPDMA_CxCR_USEIE_Pos)
#define GPDMA_CxCR_PRIO_Pos   22
#define GPDMA_CxCR_PRIO_Msk   (3UL << GPDMA_CxCR_PRIO_Pos)

/* CxTR1 bits */
#define GPDMA_CxTR1_SDW_Pos   0   /* Source data width (log2) */
#define GPDMA_CxTR1_SDW_Msk   (3UL << GPDMA_CxTR1_SDW_Pos)
#define GPDMA_CxTR1_SINC_Pos  3   /* Source increment */
#define GPDMA_CxTR1_SINC_Msk  (1UL << GPDMA_CxTR1_SINC_Pos)
#define GPDMA_CxTR1_SAP_Pos   14  /* Source allocated port (0 or 1) */
#define GPDMA_CxTR1_SAP_Msk   (1UL << GPDMA_CxTR1_SAP_Pos)
#define GPDMA_CxTR1_DDW_Pos   16  /* Destination data width (log2) */
#define GPDMA_CxTR1_DDW_Msk   (3UL << GPDMA_CxTR1_DDW_Pos)
#define GPDMA_CxTR1_DINC_Pos  19  /* Destination increment */
#define GPDMA_CxTR1_DINC_Msk  (1UL << GPDMA_CxTR1_DINC_Pos)
#define GPDMA_CxTR1_DAP_Pos   30  /* Destination allocated port (0 or 1) */
#define GPDMA_CxTR1_DAP_Msk   (1UL << GPDMA_CxTR1_DAP_Pos)

/* CxTR2 bits */
#define GPDMA_CxTR2_REQSEL_Pos  0   /* Hardware request selection */
#define GPDMA_CxTR2_REQSEL_Msk  (0x3FUL << GPDMA_CxTR2_REQSEL_Pos)
#define GPDMA_CxTR2_SWREQ_Pos   9   /* Software request (memory-to-memory) */
#define GPDMA_CxTR2_SWREQ_Msk   (1UL << GPDMA_CxTR2_SWREQ_Pos)
#define GPDMA_CxTR2_DREQ_Pos    10  /* Direction: 0=src periph, 1=dst periph */
#define GPDMA_CxTR2_DREQ_Msk    (1UL << GPDMA_CxTR2_DREQ_Pos)

#ifdef WHAL_CFG_STM32WBA_GPDMA_DIRECT_API_MAPPING
#define whal_Stm32wba_Gpdma_Init      whal_Dma_Init
#define whal_Stm32wba_Gpdma_Deinit    whal_Dma_Deinit
#define whal_Stm32wba_Gpdma_Configure whal_Dma_Configure
#define whal_Stm32wba_Gpdma_Start     whal_Dma_Start
#define whal_Stm32wba_Gpdma_Stop      whal_Dma_Stop
#endif

#ifdef WHAL_CFG_STM32WBA_GPDMA_SINGLE_INSTANCE
const whal_Dma whal_Stm32wba_Gpdma_Dev = WHAL_CFG_STM32WBA_GPDMA_DEV;
#endif

whal_Error whal_Stm32wba_Gpdma_Init(whal_Dma *dmaDev)
{
#ifdef WHAL_CFG_STM32WBA_GPDMA_SINGLE_INSTANCE
    (void)dmaDev;
#else
    if (!dmaDev || !dmaDev->cfg)
        return WHAL_EINVAL;
#endif

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Gpdma_Deinit(whal_Dma *dmaDev)
{
#ifdef WHAL_CFG_STM32WBA_GPDMA_SINGLE_INSTANCE
    const whal_Stm32wba_Gpdma_Cfg *cfg =
        (const whal_Stm32wba_Gpdma_Cfg *)whal_Stm32wba_Gpdma_Dev.cfg;
    size_t base = whal_Stm32wba_Gpdma_Dev.base;
    (void)dmaDev;
#else
    const whal_Stm32wba_Gpdma_Cfg *cfg;
    size_t base;

    if (!dmaDev || !dmaDev->cfg)
        return WHAL_EINVAL;

    cfg = (const whal_Stm32wba_Gpdma_Cfg *)dmaDev->cfg;
    base = dmaDev->base;
#endif

    /* Reset all channels */
    for (uint8_t ch = 0; ch < cfg->numChannels; ch++) {
        whal_Reg_Write(base, GPDMA_CxCR(ch),
                       GPDMA_CxCR_RESET_Msk);
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Gpdma_Configure(whal_Dma *dmaDev, size_t ch,
                                         const void *chCfg)
{
    const whal_Stm32wba_Gpdma_ChCfg *ccfg;
    size_t tr1, tr2;
    whal_Error err;
#ifdef WHAL_CFG_STM32WBA_GPDMA_SINGLE_INSTANCE
    const whal_Stm32wba_Gpdma_Cfg *cfg =
        (const whal_Stm32wba_Gpdma_Cfg *)whal_Stm32wba_Gpdma_Dev.cfg;
    size_t base = whal_Stm32wba_Gpdma_Dev.base;
    (void)dmaDev;

    if (!chCfg)
        return WHAL_EINVAL;
#else
    const whal_Stm32wba_Gpdma_Cfg *cfg;
    size_t base;

    if (!dmaDev || !dmaDev->cfg || !chCfg)
        return WHAL_EINVAL;

    cfg = (const whal_Stm32wba_Gpdma_Cfg *)dmaDev->cfg;
    base = dmaDev->base;
#endif
    ccfg = (const whal_Stm32wba_Gpdma_ChCfg *)chCfg;

    if (ch >= cfg->numChannels)
        return WHAL_EINVAL;

    if (ccfg->nbytes == 0 || ccfg->nbytes > 0xFFFF)
        return WHAL_EINVAL;

    /* Reset the channel and wait for idle */
    whal_Reg_Write(base, GPDMA_CxCR(ch), GPDMA_CxCR_RESET_Msk);
    err = whal_Reg_ReadPoll(base, GPDMA_CxSR(ch), GPDMA_CxSR_IDLEF_Msk,
                             GPDMA_CxSR_IDLEF_Msk, cfg->timeout);
    if (err)
        return err;

    /* Clear any pending flags */
    whal_Reg_Write(base, GPDMA_CxFCR(ch), GPDMA_CxFCR_ALL);

    /* Build CxTR1: source and destination widths and increment modes.
     * Route the memory side to port 1 and the peripheral side to port 0
     * so the two halves of each transfer don't serialize on the same AHB
     * port. Mem-to-mem leaves both on port 0. */
    tr1 = whal_SetBits(GPDMA_CxTR1_SDW_Msk, GPDMA_CxTR1_SDW_Pos, ccfg->srcWidth) |
          whal_SetBits(GPDMA_CxTR1_SINC_Msk, GPDMA_CxTR1_SINC_Pos, ccfg->srcInc) |
          whal_SetBits(GPDMA_CxTR1_DDW_Msk, GPDMA_CxTR1_DDW_Pos, ccfg->dstWidth) |
          whal_SetBits(GPDMA_CxTR1_DINC_Msk, GPDMA_CxTR1_DINC_Pos, ccfg->dstInc);

    if (ccfg->dir == WHAL_STM32WBA_GPDMA_DIR_MEM_TO_PERIPH)
        tr1 |= GPDMA_CxTR1_SAP_Msk;
    else if (ccfg->dir == WHAL_STM32WBA_GPDMA_DIR_PERIPH_TO_MEM)
        tr1 |= GPDMA_CxTR1_DAP_Msk;

    whal_Reg_Write(base, GPDMA_CxTR1(ch), tr1);

    /* Build CxTR2: request selection and direction */
    tr2 = 0;
    if (ccfg->dir == WHAL_STM32WBA_GPDMA_DIR_MEM_TO_MEM) {
        tr2 |= GPDMA_CxTR2_SWREQ_Msk;
    } else {
        tr2 |= whal_SetBits(GPDMA_CxTR2_REQSEL_Msk, GPDMA_CxTR2_REQSEL_Pos,
                            ccfg->reqSel);
        if (ccfg->dir == WHAL_STM32WBA_GPDMA_DIR_MEM_TO_PERIPH)
            tr2 |= GPDMA_CxTR2_DREQ_Msk;
    }
    whal_Reg_Write(base, GPDMA_CxTR2(ch), tr2);

    /* Byte count and addresses */
    whal_Reg_Write(base, GPDMA_CxBR1(ch), ccfg->nbytes);
    whal_Reg_Write(base, GPDMA_CxSAR(ch), ccfg->srcAddr);
    whal_Reg_Write(base, GPDMA_CxDAR(ch), ccfg->dstAddr);

    /* No linked-list */
    whal_Reg_Write(base, GPDMA_CxLLR(ch), 0);

    /* Enable transfer complete and error interrupts. Run the channel at
     * high priority so it isn't starved by concurrent CPU/AHB traffic. */
    whal_Reg_Write(base, GPDMA_CxCR(ch),
                   GPDMA_CxCR_TCIE_Msk | GPDMA_CxCR_DTEIE_Msk |
                   GPDMA_CxCR_ULEIE_Msk | GPDMA_CxCR_USEIE_Msk |
                   whal_SetBits(GPDMA_CxCR_PRIO_Msk, GPDMA_CxCR_PRIO_Pos, 3));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Gpdma_Start(whal_Dma *dmaDev, size_t ch)
{
#ifdef WHAL_CFG_STM32WBA_GPDMA_SINGLE_INSTANCE
    const whal_Stm32wba_Gpdma_Cfg *cfg =
        (const whal_Stm32wba_Gpdma_Cfg *)whal_Stm32wba_Gpdma_Dev.cfg;
    size_t base = whal_Stm32wba_Gpdma_Dev.base;
    (void)dmaDev;
#else
    const whal_Stm32wba_Gpdma_Cfg *cfg;
    size_t base;

    if (!dmaDev || !dmaDev->cfg)
        return WHAL_EINVAL;

    cfg = (const whal_Stm32wba_Gpdma_Cfg *)dmaDev->cfg;
    base = dmaDev->base;
#endif

    if (ch >= cfg->numChannels)
        return WHAL_EINVAL;

    /* Clear any stale flags from the previous transfer before enabling */
    whal_Reg_Write(base, GPDMA_CxFCR(ch), GPDMA_CxFCR_ALL);

    /* Set EN bit without disturbing the already-configured interrupt enables */
    whal_Reg_Update(base, GPDMA_CxCR(ch), GPDMA_CxCR_EN_Msk,
                    GPDMA_CxCR_EN_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wba_Gpdma_Stop(whal_Dma *dmaDev, size_t ch)
{
#ifdef WHAL_CFG_STM32WBA_GPDMA_SINGLE_INSTANCE
    const whal_Stm32wba_Gpdma_Cfg *cfg =
        (const whal_Stm32wba_Gpdma_Cfg *)whal_Stm32wba_Gpdma_Dev.cfg;
    size_t base = whal_Stm32wba_Gpdma_Dev.base;
    (void)dmaDev;
#else
    const whal_Stm32wba_Gpdma_Cfg *cfg;
    size_t base;

    if (!dmaDev || !dmaDev->cfg)
        return WHAL_EINVAL;

    cfg = (const whal_Stm32wba_Gpdma_Cfg *)dmaDev->cfg;
    base = dmaDev->base;
#endif

    if (ch >= cfg->numChannels)
        return WHAL_EINVAL;

    whal_Reg_Update(base, GPDMA_CxCR(ch), GPDMA_CxCR_EN_Msk, 0);

    whal_Reg_Write(base, GPDMA_CxFCR(ch), GPDMA_CxFCR_ALL);

    return WHAL_SUCCESS;
}

void whal_Stm32wba_Gpdma_IRQHandler(whal_Dma *dmaDev, size_t ch,
                                    whal_Stm32wba_Gpdma_Callback cb, void *ctx)
{
    size_t sr;
#ifdef WHAL_CFG_STM32WBA_GPDMA_SINGLE_INSTANCE
    size_t base = whal_Stm32wba_Gpdma_Dev.base;
    (void)dmaDev;
#else
    size_t base;

    if (!dmaDev)
        return;

    base = dmaDev->base;
#endif
    sr = whal_Reg_Read(base, GPDMA_CxSR(ch));

    if (sr & GPDMA_CxSR_ALL_ERR) {
        whal_Reg_Write(base, GPDMA_CxFCR(ch), GPDMA_CxSR_ALL_ERR);
        if (cb)
            cb(ctx, WHAL_EHARDWARE);
        return;
    }

    if (sr & GPDMA_CxSR_TCF_Msk) {
        whal_Reg_Write(base, GPDMA_CxFCR(ch), GPDMA_CxSR_TCF_Msk);
        if (cb)
            cb(ctx, WHAL_SUCCESS);
    }

    if (sr & GPDMA_CxSR_HTF_Msk) {
        whal_Reg_Write(base, GPDMA_CxFCR(ch), GPDMA_CxSR_HTF_Msk);
    }
}

#ifndef WHAL_CFG_STM32WBA_GPDMA_DIRECT_API_MAPPING
const whal_DmaDriver whal_Stm32wba_Gpdma_Driver = {
    .Init = whal_Stm32wba_Gpdma_Init,
    .Deinit = whal_Stm32wba_Gpdma_Deinit,
    .Configure = whal_Stm32wba_Gpdma_Configure,
    .Start = whal_Stm32wba_Gpdma_Start,
    .Stop = whal_Stm32wba_Gpdma_Stop,
};
#endif
