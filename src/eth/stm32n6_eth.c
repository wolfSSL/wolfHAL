/* stm32n6_eth.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32N6_ETH_DEV initializer */
#include <wolfHAL/eth/stm32n6_eth.h>
#include <wolfHAL/eth/eth.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

const whal_Eth whal_Stm32n6_Eth_Dev = WHAL_CFG_STM32N6_ETH_DEV;

/* Mutable ring-tracking state lives here, separate from the const cfg. */
static struct {
    size_t txHead;
    size_t rxHead;
} eth_state;

/**
 * @file stm32n6_eth.c
 * @brief STM32N6 Ethernet MAC driver implementation.
 *
 * The STM32N6 uses the Synopsys DWC EQOS GMAC with AXI 64-bit bus.
 * Register layout matches the STM32H5 except for:
 * - AXI coherency registers at DMA offsets 0x1020-0x1028
 * - 2 DMA channels at 0x80 stride (this driver uses channel 0 only)
 * - Descriptor alignment: bits 2:0 reserved (8-byte aligned)
 *
 * ETH1 base: 0x48036000
 *   MAC: 0x0000   MTL: 0x0C00   DMA: 0x1000
 */

/* --- MAC registers --- */
#define ETH_MACCR_REG       0x0000
#define ETH_MACCR_RE_Pos    0
#define ETH_MACCR_RE_Msk    (1UL << ETH_MACCR_RE_Pos)
#define ETH_MACCR_TE_Pos    1
#define ETH_MACCR_TE_Msk    (1UL << ETH_MACCR_TE_Pos)
#define ETH_MACCR_DM_Pos    13
#define ETH_MACCR_DM_Msk    (1UL << ETH_MACCR_DM_Pos)
#define ETH_MACCR_FES_Pos   14
#define ETH_MACCR_FES_Msk   (1UL << ETH_MACCR_FES_Pos)
#define ETH_MACCR_PS_Pos    15
#define ETH_MACCR_PS_Msk    (1UL << ETH_MACCR_PS_Pos)
#define ETH_MACCR_IPC_Pos   27
#define ETH_MACCR_IPC_Msk   (1UL << ETH_MACCR_IPC_Pos)

#define ETH_MACPFR_REG      0x0008

#define ETH_MACRXQC0R_REG     0x00A0
#define ETH_MACRXQC0R_RXQ0EN_Pos 0
#define ETH_MACRXQC0R_RXQ0EN_Msk (3UL << ETH_MACRXQC0R_RXQ0EN_Pos)
#define ETH_MACRXQC0R_RXQ0EN_GEN 0x2 /* Enable RxQ0 for generic traffic */

#define ETH_MACA0HR_REG     0x0300
#define ETH_MACA0LR_REG     0x0304

#define ETH_MACMDIOAR_REG   0x0200
#define ETH_MACMDIOAR_MB_Pos  0
#define ETH_MACMDIOAR_MB_Msk  (1UL << ETH_MACMDIOAR_MB_Pos)
#define ETH_MACMDIOAR_GOC_Pos 2
#define ETH_MACMDIOAR_GOC_Msk (3UL << ETH_MACMDIOAR_GOC_Pos)
#define ETH_MACMDIOAR_CR_Pos  8
#define ETH_MACMDIOAR_CR_Msk  (0xFUL << ETH_MACMDIOAR_CR_Pos)
#define ETH_MACMDIOAR_RDA_Pos 16
#define ETH_MACMDIOAR_RDA_Msk (0x1FUL << ETH_MACMDIOAR_RDA_Pos)
#define ETH_MACMDIOAR_PA_Pos  21
#define ETH_MACMDIOAR_PA_Msk  (0x1FUL << ETH_MACMDIOAR_PA_Pos)

#define ETH_MACMDIODR_REG   0x0204
#define ETH_MACMDIODR_MD_Msk 0xFFFFUL

#define ETH_MDIO_GOC_WRITE  0x1
#define ETH_MDIO_GOC_READ   0x3

/* --- MTL registers --- */
#define ETH_MTLTXQOMR_REG   0x0D00
#define ETH_MTLTXQOMR_TSF_Pos  1
#define ETH_MTLTXQOMR_TSF_Msk  (1UL << ETH_MTLTXQOMR_TSF_Pos)
#define ETH_MTLTXQOMR_TXQEN_Pos 2
#define ETH_MTLTXQOMR_TXQEN_Msk (3UL << ETH_MTLTXQOMR_TXQEN_Pos)

#define ETH_MTLRXQOMR_REG   0x0D30
#define ETH_MTLRXQOMR_RSF_Pos  5
#define ETH_MTLRXQOMR_RSF_Msk  (1UL << ETH_MTLRXQOMR_RSF_Pos)

/* --- DMA registers --- */
#define ETH_DMAMR_REG        0x1000
#define ETH_DMAMR_SWR_Pos    0
#define ETH_DMAMR_SWR_Msk    (1UL << ETH_DMAMR_SWR_Pos)

#define ETH_DMASBMR_REG     0x1004
#define ETH_DMASBMR_FB_Pos  0
#define ETH_DMASBMR_FB_Msk  (1UL << ETH_DMASBMR_FB_Pos)
#define ETH_DMASBMR_AAL_Pos 12
#define ETH_DMASBMR_AAL_Msk (1UL << ETH_DMASBMR_AAL_Pos)

/* AXI coherency registers (new on N6, not on H5) */
#define ETH_DMAA4TXACR_REG  0x1020
#define ETH_DMAA4RXACR_REG  0x1024
#define ETH_DMAA4DACR_REG   0x1028

/* Channel 0 DMA registers (stride 0x80 per channel) */
#define ETH_DMAC0CR_REG      0x1100

#define ETH_DMAC0TXCR_REG   0x1104
#define ETH_DMAC0TXCR_ST_Pos  0
#define ETH_DMAC0TXCR_ST_Msk  (1UL << ETH_DMAC0TXCR_ST_Pos)
#define ETH_DMAC0TXCR_TXPBL_Pos 16
#define ETH_DMAC0TXCR_TXPBL_Msk (0x3FUL << ETH_DMAC0TXCR_TXPBL_Pos)

#define ETH_DMAC0RXCR_REG   0x1108
#define ETH_DMAC0RXCR_SR_Pos  0
#define ETH_DMAC0RXCR_SR_Msk  (1UL << ETH_DMAC0RXCR_SR_Pos)
#define ETH_DMAC0RXCR_RBSZ_Pos 1
#define ETH_DMAC0RXCR_RBSZ_Msk (0x3FFFUL << ETH_DMAC0RXCR_RBSZ_Pos)
#define ETH_DMAC0RXCR_RXPBL_Pos 16
#define ETH_DMAC0RXCR_RXPBL_Msk (0x3FUL << ETH_DMAC0RXCR_RXPBL_Pos)

#define ETH_DMAC0TXDLAR_REG 0x1114
#define ETH_DMAC0RXDLAR_REG 0x111C
#define ETH_DMAC0TXDTPR_REG 0x1120
#define ETH_DMAC0RXDTPR_REG 0x1128
#define ETH_DMAC0TXRLR_REG  0x112C
#define ETH_DMAC0RXRLR_REG  0x1130

/* --- TX descriptor bits (TDES3) --- */
#define TDES3_OWN  (1UL << 31)
#define TDES3_FD   (1UL << 29)
#define TDES3_LD   (1UL << 28)

/* TX descriptor bits (TDES2) */
#define TDES2_IOC  (1UL << 31)

/* --- RX descriptor bits (RDES3) --- */
#define RDES3_OWN   (1UL << 31)
#define RDES3_IOC   (1UL << 30)
#define RDES3_BUF1V (1UL << 24)
#define RDES3_ES    (1UL << 15)
#define RDES3_PL_Msk 0x7FFFUL

/* Default burst length */
#define ETH_PBL    32

#ifdef WHAL_CFG_STM32N6_ETH_DIRECT_API_MAPPING
#define whal_Stm32n6_Eth_Init      whal_Eth_Init
#define whal_Stm32n6_Eth_Deinit    whal_Eth_Deinit
#define whal_Stm32n6_Eth_Start     whal_Eth_Start
#define whal_Stm32n6_Eth_Stop      whal_Eth_Stop
#define whal_Stm32n6_Eth_Send      whal_Eth_Send
#define whal_Stm32n6_Eth_Recv      whal_Eth_Recv
#define whal_Stm32n6_Eth_MdioRead  whal_Eth_MdioRead
#define whal_Stm32n6_Eth_MdioWrite whal_Eth_MdioWrite
#endif /* WHAL_CFG_STM32N6_ETH_DIRECT_API_MAPPING */

static whal_Error MdioPoll(size_t base, whal_Timeout *timeout)
{
    return whal_Reg_ReadPoll(base, ETH_MACMDIOAR_REG,
                             ETH_MACMDIOAR_MB_Msk, 0, timeout);
}

whal_Error whal_Stm32n6_Eth_Init(whal_Eth *ethDev)
{
    const whal_Stm32n6_Eth_Cfg *cfg =
        (const whal_Stm32n6_Eth_Cfg *)whal_Stm32n6_Eth_Dev.cfg;
    size_t base = whal_Stm32n6_Eth_Dev.base;
    whal_Error err;
    (void)ethDev;

    if (!cfg->txDescs || !cfg->txBufs || cfg->txDescCount == 0 ||
        !cfg->rxDescs || !cfg->rxBufs || cfg->rxDescCount == 0 ||
        cfg->txBufSize == 0 || cfg->rxBufSize == 0)
        return WHAL_EINVAL;

    /* DMA software reset */
    whal_Reg_Update(base, ETH_DMAMR_REG, ETH_DMAMR_SWR_Msk,
                    ETH_DMAMR_SWR_Msk);
    err = whal_Reg_ReadPoll(base, ETH_DMAMR_REG, ETH_DMAMR_SWR_Msk, 0,
                            cfg->timeout);
    if (err)
        return err;

    /* DMA bus mode: fixed burst, address-aligned */
    whal_Reg_Write(base, ETH_DMASBMR_REG,
                   ETH_DMASBMR_FB_Msk | ETH_DMASBMR_AAL_Msk);

    /* AXI coherency registers: use default non-cacheable settings */
    whal_Reg_Write(base, ETH_DMAA4TXACR_REG, 0);
    whal_Reg_Write(base, ETH_DMAA4RXACR_REG, 0);
    whal_Reg_Write(base, ETH_DMAA4DACR_REG, 0);

    /* DMA channel 0: contiguous descriptors (DSL=0) */
    whal_Reg_Write(base, ETH_DMAC0CR_REG, 0);

    /* DMA TX: burst length, not started yet */
    whal_Reg_Write(base, ETH_DMAC0TXCR_REG,
                   whal_SetBits(ETH_DMAC0TXCR_TXPBL_Msk,
                                ETH_DMAC0TXCR_TXPBL_Pos, ETH_PBL));

    /* DMA RX: burst length, buffer size, not started yet */
    whal_Reg_Write(base, ETH_DMAC0RXCR_REG,
                   whal_SetBits(ETH_DMAC0RXCR_RXPBL_Msk,
                                ETH_DMAC0RXCR_RXPBL_Pos, ETH_PBL) |
                   whal_SetBits(ETH_DMAC0RXCR_RBSZ_Msk,
                                ETH_DMAC0RXCR_RBSZ_Pos, cfg->rxBufSize));

    /* Set up TX descriptor ring */
    for (size_t i = 0; i < cfg->txDescCount; i++) {
        cfg->txDescs[i].des[0] = 0;
        cfg->txDescs[i].des[1] = 0;
        cfg->txDescs[i].des[2] = 0;
        cfg->txDescs[i].des[3] = 0;
    }

    /* Set up RX descriptor ring with pre-allocated buffers */
    for (size_t i = 0; i < cfg->rxDescCount; i++) {
        cfg->rxDescs[i].des[0] = (uintptr_t)(cfg->rxBufs + i * cfg->rxBufSize);
        cfg->rxDescs[i].des[1] = 0;
        cfg->rxDescs[i].des[2] = 0;
        cfg->rxDescs[i].des[3] = RDES3_OWN | RDES3_IOC | RDES3_BUF1V;
    }

    /* Program descriptor ring addresses and lengths */
    whal_Reg_Write(base, ETH_DMAC0TXDLAR_REG, (uintptr_t)cfg->txDescs);
    whal_Reg_Write(base, ETH_DMAC0RXDLAR_REG, (uintptr_t)cfg->rxDescs);
    whal_Reg_Write(base, ETH_DMAC0TXRLR_REG, cfg->txDescCount - 1);
    whal_Reg_Write(base, ETH_DMAC0RXRLR_REG, cfg->rxDescCount - 1);

    /* MTL: store-and-forward for TX and RX, enable TX queue */
    whal_Reg_Update(base, ETH_MTLTXQOMR_REG,
                    ETH_MTLTXQOMR_TSF_Msk | ETH_MTLTXQOMR_TXQEN_Msk,
                    ETH_MTLTXQOMR_TSF_Msk |
                    whal_SetBits(ETH_MTLTXQOMR_TXQEN_Msk,
                                 ETH_MTLTXQOMR_TXQEN_Pos, 2));
    whal_Reg_Update(base, ETH_MTLRXQOMR_REG,
                    ETH_MTLRXQOMR_RSF_Msk, ETH_MTLRXQOMR_RSF_Msk);

    /* Enable MAC Rx queue 0 for generic traffic. Reset value disables
     * all Rx queues, so without this the MAC drops every received
     * packet before it ever reaches the MTL FIFO / DMA. */
    whal_Reg_Update(base, ETH_MACRXQC0R_REG, ETH_MACRXQC0R_RXQ0EN_Msk,
                    whal_SetBits(ETH_MACRXQC0R_RXQ0EN_Msk,
                                 ETH_MACRXQC0R_RXQ0EN_Pos,
                                 ETH_MACRXQC0R_RXQ0EN_GEN));

    /* MAC address */
    whal_Reg_Write(base, ETH_MACA0LR_REG,
                   ((uint32_t)whal_Stm32n6_Eth_Dev.macAddr[3] << 24) |
                   ((uint32_t)whal_Stm32n6_Eth_Dev.macAddr[2] << 16) |
                   ((uint32_t)whal_Stm32n6_Eth_Dev.macAddr[1] << 8) |
                   ((uint32_t)whal_Stm32n6_Eth_Dev.macAddr[0]));
    whal_Reg_Write(base, ETH_MACA0HR_REG,
                   ((uint32_t)whal_Stm32n6_Eth_Dev.macAddr[5] << 8) |
                   ((uint32_t)whal_Stm32n6_Eth_Dev.macAddr[4]));

    /* Reset ring tracking state */
    eth_state.txHead = 0;
    eth_state.rxHead = 0;

    /* Ensure descriptor rings (Normal memory) are visible before any
     * subsequent peripheral kick from Start(). */
    __asm__ volatile ("dsb sy" ::: "memory");

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32n6_Eth_Deinit(whal_Eth *ethDev)
{
    (void)ethDev;

    whal_Reg_Update(whal_Stm32n6_Eth_Dev.base, ETH_DMAMR_REG,
                    ETH_DMAMR_SWR_Msk, ETH_DMAMR_SWR_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32n6_Eth_Start(whal_Eth *ethDev, uint8_t speed,
                                  uint8_t duplex)
{
    const whal_Stm32n6_Eth_Cfg *cfg =
        (const whal_Stm32n6_Eth_Cfg *)whal_Stm32n6_Eth_Dev.cfg;
    size_t base = whal_Stm32n6_Eth_Dev.base;
    (void)ethDev;

    /* Configure MAC speed and duplex to match PHY. PS=1 selects the
     * MII/RMII data path; required because the EQOS MAC defaults to GMII
     * (PS=0) and this driver only supports 10/100 Mbps. */
    whal_Reg_Update(base, ETH_MACCR_REG,
                    ETH_MACCR_PS_Msk | ETH_MACCR_FES_Msk | ETH_MACCR_DM_Msk,
                    ETH_MACCR_PS_Msk |
                    ((speed == 100) ? ETH_MACCR_FES_Msk : 0) |
                    (duplex ? ETH_MACCR_DM_Msk : 0));

    /* Enable MAC TX and RX */
    whal_Reg_Update(base, ETH_MACCR_REG,
                    ETH_MACCR_TE_Msk | ETH_MACCR_RE_Msk,
                    ETH_MACCR_TE_Msk | ETH_MACCR_RE_Msk);

    /* Start DMA TX */
    whal_Reg_Update(base, ETH_DMAC0TXCR_REG,
                    ETH_DMAC0TXCR_ST_Msk, ETH_DMAC0TXCR_ST_Msk);

    /* Start DMA RX */
    whal_Reg_Update(base, ETH_DMAC0RXCR_REG,
                    ETH_DMAC0RXCR_SR_Msk, ETH_DMAC0RXCR_SR_Msk);

    /* Kick RX DMA by writing tail pointer past last descriptor */
    whal_Reg_Write(base, ETH_DMAC0RXDTPR_REG,
                   (uintptr_t)&cfg->rxDescs[cfg->rxDescCount]);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32n6_Eth_Stop(whal_Eth *ethDev)
{
    size_t base = whal_Stm32n6_Eth_Dev.base;
    (void)ethDev;

    /* Stop DMA TX */
    whal_Reg_Update(base, ETH_DMAC0TXCR_REG, ETH_DMAC0TXCR_ST_Msk, 0);

    /* Stop DMA RX */
    whal_Reg_Update(base, ETH_DMAC0RXCR_REG, ETH_DMAC0RXCR_SR_Msk, 0);

    /* Disable MAC TX and RX */
    whal_Reg_Update(base, ETH_MACCR_REG,
                    ETH_MACCR_TE_Msk | ETH_MACCR_RE_Msk, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32n6_Eth_Send(whal_Eth *ethDev, const void *frame,
                                 size_t len)
{
    const uint8_t *frameBuf = (const uint8_t *)frame;
    const whal_Stm32n6_Eth_Cfg *cfg =
        (const whal_Stm32n6_Eth_Cfg *)whal_Stm32n6_Eth_Dev.cfg;
    size_t base = whal_Stm32n6_Eth_Dev.base;
    whal_Stm32n6_Eth_TxDesc *desc;
    size_t idx;
    (void)ethDev;

    if (!frame || len == 0)
        return WHAL_EINVAL;

    if (len > cfg->txBufSize)
        return WHAL_EINVAL;
    idx = eth_state.txHead;
    desc = &cfg->txDescs[idx];

    /* Check if descriptor is available (OWN must be 0) */
    if (desc->des[3] & TDES3_OWN)
        return WHAL_ENOTREADY;

    /* Copy frame into TX buffer */
    uint8_t *txBuf = cfg->txBufs + idx * cfg->txBufSize;
    for (size_t i = 0; i < len; i++)
        txBuf[i] = frameBuf[i];

    /* Set up descriptor */
    desc->des[0] = (uintptr_t)txBuf;
    desc->des[1] = 0;
    desc->des[2] = (len & 0x3FFF) | TDES2_IOC;
    desc->des[3] = TDES3_OWN | TDES3_FD | TDES3_LD | (len & 0x7FFF);

    /* Advance ring position */
    eth_state.txHead = (idx + 1) % cfg->txDescCount;

    /* Order descriptor writes (Normal memory) before the tail-pointer kick
     * (Device memory). The M55's write buffer can otherwise let the DMA
     * fetch the descriptor before OWN=1 has drained to AXI SRAM. */
    __asm__ volatile ("dsb sy" ::: "memory");

    /* Kick DMA */
    whal_Reg_Write(base, ETH_DMAC0TXDTPR_REG,
                   (uintptr_t)&cfg->txDescs[cfg->txDescCount]);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32n6_Eth_Recv(whal_Eth *ethDev, void *frame,
                                 size_t *len)
{
    uint8_t *frameBuf = (uint8_t *)frame;
    const whal_Stm32n6_Eth_Cfg *cfg =
        (const whal_Stm32n6_Eth_Cfg *)whal_Stm32n6_Eth_Dev.cfg;
    size_t base = whal_Stm32n6_Eth_Dev.base;
    whal_Stm32n6_Eth_RxDesc *desc;
    size_t idx;
    uint32_t rdes3;
    size_t pktLen;
    (void)ethDev;

    if (!frame || !len)
        return WHAL_EINVAL;

    idx = eth_state.rxHead;
    desc = &cfg->rxDescs[idx];

    rdes3 = desc->des[3];

    /* Check if DMA has released this descriptor */
    if (rdes3 & RDES3_OWN)
        return WHAL_ENOTREADY;

    /* Check for errors */
    if (rdes3 & RDES3_ES) {
        desc->des[0] = (uintptr_t)(cfg->rxBufs + idx * cfg->rxBufSize);
        desc->des[3] = RDES3_OWN | RDES3_IOC | RDES3_BUF1V;
        eth_state.rxHead = (idx + 1) % cfg->rxDescCount;
        whal_Reg_Write(base, ETH_DMAC0RXDTPR_REG,
                       (uintptr_t)&cfg->rxDescs[cfg->rxDescCount]);
        return WHAL_EHARDWARE;
    }

    /* Extract packet length */
    pktLen = rdes3 & RDES3_PL_Msk;
    if (pktLen > *len) {
        *len = pktLen;
        return WHAL_EINVAL;
    }

    /* Copy frame data */
    uint8_t *rxBuf = (uint8_t *)(cfg->rxBufs + idx * cfg->rxBufSize);
    for (size_t i = 0; i < pktLen; i++)
        frameBuf[i] = rxBuf[i];
    *len = pktLen;

    /* Re-arm descriptor for DMA */
    desc->des[0] = (uintptr_t)rxBuf;
    desc->des[1] = 0;
    desc->des[2] = 0;
    desc->des[3] = RDES3_OWN | RDES3_IOC | RDES3_BUF1V;

    /* Advance ring position */
    eth_state.rxHead = (idx + 1) % cfg->rxDescCount;

    /* Order the OWN=1 re-arm write before the tail-pointer kick. */
    __asm__ volatile ("dsb sy" ::: "memory");

    /* Update RX tail pointer */
    whal_Reg_Write(base, ETH_DMAC0RXDTPR_REG,
                   (uintptr_t)&cfg->rxDescs[cfg->rxDescCount]);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32n6_Eth_MdioRead(whal_Eth *ethDev, uint8_t phyAddr,
                                      uint8_t reg, uint16_t *val)
{
    const whal_Stm32n6_Eth_Cfg *cfg =
        (const whal_Stm32n6_Eth_Cfg *)whal_Stm32n6_Eth_Dev.cfg;
    size_t base = whal_Stm32n6_Eth_Dev.base;
    whal_Error err;
    (void)ethDev;

    if (!val)
        return WHAL_EINVAL;

    err = MdioPoll(base, cfg->timeout);
    if (err)
        return err;

    whal_Reg_Write(base, ETH_MACMDIOAR_REG,
                   whal_SetBits(ETH_MACMDIOAR_PA_Msk, ETH_MACMDIOAR_PA_Pos,
                                phyAddr) |
                   whal_SetBits(ETH_MACMDIOAR_RDA_Msk, ETH_MACMDIOAR_RDA_Pos,
                                reg) |
                   whal_SetBits(ETH_MACMDIOAR_CR_Msk, ETH_MACMDIOAR_CR_Pos,
                                cfg->mdioCr) |
                   whal_SetBits(ETH_MACMDIOAR_GOC_Msk, ETH_MACMDIOAR_GOC_Pos,
                                ETH_MDIO_GOC_READ) |
                   ETH_MACMDIOAR_MB_Msk);

    err = MdioPoll(base, cfg->timeout);
    if (err)
        return err;

    *val = (uint16_t)(whal_Reg_Read(base, ETH_MACMDIODR_REG) &
                      ETH_MACMDIODR_MD_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32n6_Eth_MdioWrite(whal_Eth *ethDev, uint8_t phyAddr,
                                       uint8_t reg, uint16_t val)
{
    const whal_Stm32n6_Eth_Cfg *cfg =
        (const whal_Stm32n6_Eth_Cfg *)whal_Stm32n6_Eth_Dev.cfg;
    size_t base = whal_Stm32n6_Eth_Dev.base;
    whal_Error err;
    (void)ethDev;

    err = MdioPoll(base, cfg->timeout);
    if (err)
        return err;

    whal_Reg_Write(base, ETH_MACMDIODR_REG, val);

    whal_Reg_Write(base, ETH_MACMDIOAR_REG,
                   whal_SetBits(ETH_MACMDIOAR_PA_Msk, ETH_MACMDIOAR_PA_Pos,
                                phyAddr) |
                   whal_SetBits(ETH_MACMDIOAR_RDA_Msk, ETH_MACMDIOAR_RDA_Pos,
                                reg) |
                   whal_SetBits(ETH_MACMDIOAR_CR_Msk, ETH_MACMDIOAR_CR_Pos,
                                cfg->mdioCr) |
                   whal_SetBits(ETH_MACMDIOAR_GOC_Msk, ETH_MACMDIOAR_GOC_Pos,
                                ETH_MDIO_GOC_WRITE) |
                   ETH_MACMDIOAR_MB_Msk);

    err = MdioPoll(base, cfg->timeout);
    if (err)
        return err;

    return WHAL_SUCCESS;
}

#define ETH_MACCR_LM_Pos    12
#define ETH_MACCR_LM_Msk    (1UL << ETH_MACCR_LM_Pos)

whal_Error whal_Stm32n6_Eth_Ext_EnableLoopback(whal_Eth *ethDev,
                                                uint8_t enable)
{
    (void)ethDev;

    whal_Reg_Update(whal_Stm32n6_Eth_Dev.base, ETH_MACCR_REG, ETH_MACCR_LM_Msk,
                    whal_SetBits(ETH_MACCR_LM_Msk, ETH_MACCR_LM_Pos,
                                 enable ? 1 : 0));

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32N6_ETH_DIRECT_API_MAPPING
const whal_EthDriver whal_Stm32n6_Eth_Driver = {
    .Init = whal_Stm32n6_Eth_Init,
    .Deinit = whal_Stm32n6_Eth_Deinit,
    .Start = whal_Stm32n6_Eth_Start,
    .Stop = whal_Stm32n6_Eth_Stop,
    .Send = whal_Stm32n6_Eth_Send,
    .Recv = whal_Stm32n6_Eth_Recv,
    .MdioRead = whal_Stm32n6_Eth_MdioRead,
    .MdioWrite = whal_Stm32n6_Eth_MdioWrite,
};
#endif /* !WHAL_CFG_STM32N6_ETH_DIRECT_API_MAPPING */
