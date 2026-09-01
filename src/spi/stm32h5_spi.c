/* stm32h5_spi.c
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
#ifdef WHAL_CFG_STM32H5_SPI_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_Stm32h5_Spi_Dev singleton (possibly via platform alias macro) */
#endif
#include <wolfHAL/spi/stm32h5_spi.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * STM32H5 SPI Register Definitions
 *
 * The H5 SPI peripheral uses separate configuration registers (CFG1, CFG2)
 * and has TX/RX FIFOs with configurable thresholds. Master transfers
 * require explicit CSTART to begin clocking.
 */

/* Control Register 1 */
#define SPI_CR1_REG 0x000
#define SPI_CR1_SPE_Pos 0
#define SPI_CR1_SPE_Msk (1UL << SPI_CR1_SPE_Pos)

#define SPI_CR1_CSTART_Pos 9
#define SPI_CR1_CSTART_Msk (1UL << SPI_CR1_CSTART_Pos)

#define SPI_CR1_SSI_Pos 12
#define SPI_CR1_SSI_Msk (1UL << SPI_CR1_SSI_Pos)

/* Control Register 2 */
#define SPI_CR2_REG 0x004
#define SPI_CR2_TSIZE_Pos 0
#define SPI_CR2_TSIZE_Msk (WHAL_BITMASK(16) << SPI_CR2_TSIZE_Pos)

/* Configuration Register 1 */
#define SPI_CFG1_REG 0x008
#define SPI_CFG1_DSIZE_Pos 0
#define SPI_CFG1_DSIZE_Msk (WHAL_BITMASK(5) << SPI_CFG1_DSIZE_Pos)

#define SPI_CFG1_FTHLV_Pos 5
#define SPI_CFG1_FTHLV_Msk (WHAL_BITMASK(4) << SPI_CFG1_FTHLV_Pos)

#define SPI_CFG1_MBR_Pos 28
#define SPI_CFG1_MBR_Msk (WHAL_BITMASK(3) << SPI_CFG1_MBR_Pos)

/* Configuration Register 2 */
#define SPI_CFG2_REG 0x00C
#define SPI_CFG2_MSSI_Pos 0
#define SPI_CFG2_MSSI_Msk (WHAL_BITMASK(4) << SPI_CFG2_MSSI_Pos)

#define SPI_CFG2_COMM_Pos 17
#define SPI_CFG2_COMM_Msk (WHAL_BITMASK(2) << SPI_CFG2_COMM_Pos)

#define SPI_CFG2_MASTER_Pos 22
#define SPI_CFG2_MASTER_Msk (1UL << SPI_CFG2_MASTER_Pos)

#define SPI_CFG2_LSBFRST_Pos 23
#define SPI_CFG2_LSBFRST_Msk (1UL << SPI_CFG2_LSBFRST_Pos)

#define SPI_CFG2_CPHA_Pos 24
#define SPI_CFG2_CPHA_Msk (1UL << SPI_CFG2_CPHA_Pos)

#define SPI_CFG2_CPOL_Pos 25
#define SPI_CFG2_CPOL_Msk (1UL << SPI_CFG2_CPOL_Pos)

#define SPI_CFG2_SSM_Pos 26
#define SPI_CFG2_SSM_Msk (1UL << SPI_CFG2_SSM_Pos)

/* Status Register */
#define SPI_SR_REG 0x014
#define SPI_SR_RXP_Pos 0
#define SPI_SR_RXP_Msk (1UL << SPI_SR_RXP_Pos)

#define SPI_SR_TXP_Pos 1
#define SPI_SR_TXP_Msk (1UL << SPI_SR_TXP_Pos)

#define SPI_SR_EOT_Pos 3
#define SPI_SR_EOT_Msk (1UL << SPI_SR_EOT_Pos)

#define SPI_SR_TXC_Pos 12
#define SPI_SR_TXC_Msk (1UL << SPI_SR_TXC_Pos)

/* Interrupt/Status Flags Clear Register */
#define SPI_IFCR_REG 0x018
#define SPI_IFCR_EOTC_Pos 3
#define SPI_IFCR_EOTC_Msk (1UL << SPI_IFCR_EOTC_Pos)

#define SPI_IFCR_TXTFC_Pos 4
#define SPI_IFCR_TXTFC_Msk (1UL << SPI_IFCR_TXTFC_Pos)

/* Data Registers - 8/16/32-bit accessible */
#define SPI_TXDR_REG 0x020
#define SPI_RXDR_REG 0x030

#ifdef WHAL_CFG_STM32H5_SPI_DIRECT_API_MAPPING
#define whal_Stm32h5_Spi_Init     whal_Spi_Init
#define whal_Stm32h5_Spi_Deinit   whal_Spi_Deinit
#define whal_Stm32h5_Spi_StartCom whal_Spi_StartCom
#define whal_Stm32h5_Spi_EndCom   whal_Spi_EndCom
#define whal_Stm32h5_Spi_SendRecv whal_Spi_SendRecv
#endif /* WHAL_CFG_STM32H5_SPI_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_STM32H5_SPI_SINGLE_INSTANCE
const whal_Spi whal_Stm32h5_Spi_Dev = WHAL_CFG_STM32H5_SPI_DEV;
#endif

/*
 * Calculate the baud rate prescaler index for a target baud rate.
 * SPI baud rate = fPCLK / (2 ^ (MBR + 1))
 *   MBR=0 -> /2, MBR=1 -> /4, ..., MBR=7 -> /256
 */
static uint32_t Stm32h5_Spi_CalcMbr(size_t pclk, uint32_t targetBaud)
{
    uint32_t mbr;

    for (mbr = 0; mbr < 7; mbr++) {
        if ((pclk / (2u << mbr)) <= targetBaud)
            return mbr;
    }

    return 7;
}

whal_Error whal_Stm32h5_Spi_Init(whal_Spi *spiDev)
{
#ifdef WHAL_CFG_STM32H5_SPI_SINGLE_INSTANCE
    size_t base = whal_Stm32h5_Spi_Dev.base;
    (void)spiDev;
#else
    size_t base;

    if (!spiDev || !spiDev->cfg)
        return WHAL_EINVAL;

    base = spiDev->base;
#endif

    /* Disable SPI before configuring */
    whal_Reg_Update(base, SPI_CR1_REG, SPI_CR1_SPE_Msk,
                    whal_SetBits(SPI_CR1_SPE_Msk, SPI_CR1_SPE_Pos, 0));

    whal_Reg_Update(base, SPI_CR1_REG, SPI_CR1_SSI_Msk,
                    whal_SetBits(SPI_CR1_SSI_Msk, SPI_CR1_SSI_Pos, 1));

    /* Master mode, software slave management, SSI high */
    whal_Reg_Update(base, SPI_CFG2_REG,
                    SPI_CFG2_MASTER_Msk | SPI_CFG2_SSM_Msk,
                    whal_SetBits(SPI_CFG2_MASTER_Msk, SPI_CFG2_MASTER_Pos, 1) |
                    whal_SetBits(SPI_CFG2_SSM_Msk, SPI_CFG2_SSM_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32h5_Spi_Deinit(whal_Spi *spiDev)
{
#ifdef WHAL_CFG_STM32H5_SPI_SINGLE_INSTANCE
    size_t base = whal_Stm32h5_Spi_Dev.base;
    (void)spiDev;
#else
    size_t base;

    if (!spiDev || !spiDev->cfg)
        return WHAL_EINVAL;

    base = spiDev->base;
#endif

    whal_Reg_Update(base, SPI_CR1_REG, SPI_CR1_SPE_Msk,
                    whal_SetBits(SPI_CR1_SPE_Msk, SPI_CR1_SPE_Pos, 0));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32h5_Spi_StartCom(whal_Spi *spiDev, whal_Spi_ComCfg *comCfg)
{
    uint32_t cpol, cpha, mbr, dsize, fthlv;
#ifdef WHAL_CFG_STM32H5_SPI_SINGLE_INSTANCE
    whal_Stm32h5_Spi_Cfg *cfg =
        (whal_Stm32h5_Spi_Cfg *)whal_Stm32h5_Spi_Dev.cfg;
    size_t base = whal_Stm32h5_Spi_Dev.base;
    (void)spiDev;

    if (!comCfg)
        return WHAL_EINVAL;
#else
    size_t base;
    whal_Stm32h5_Spi_Cfg *cfg;

    if (!spiDev || !spiDev->cfg || !comCfg)
        return WHAL_EINVAL;
#endif

    if (comCfg->wordSz < 4 || comCfg->wordSz > 32)
        return WHAL_EINVAL;

    if (comCfg->mode > 3 || comCfg->dataLines != 1 || comCfg->freq == 0)
        return WHAL_EINVAL;

#ifndef WHAL_CFG_STM32H5_SPI_SINGLE_INSTANCE
    base = spiDev->base;
    cfg = (whal_Stm32h5_Spi_Cfg *)spiDev->cfg;
#endif

    /* Disable SPE before reconfiguring */
    whal_Reg_Update(base, SPI_CR1_REG, SPI_CR1_SPE_Msk,
                    whal_SetBits(SPI_CR1_SPE_Msk, SPI_CR1_SPE_Pos, 0));

    mbr = Stm32h5_Spi_CalcMbr(cfg->pclk, comCfg->freq);
    cpol = (comCfg->mode >> 1) & 1;
    cpha = comCfg->mode & 1;
    dsize = comCfg->wordSz - 1;
    fthlv = 0; /* 1 data frame threshold */

    /* Set baud rate, data size, FIFO threshold */
    whal_Reg_Update(base, SPI_CFG1_REG,
                    SPI_CFG1_MBR_Msk | SPI_CFG1_DSIZE_Msk | SPI_CFG1_FTHLV_Msk,
                    whal_SetBits(SPI_CFG1_MBR_Msk, SPI_CFG1_MBR_Pos, mbr) |
                    whal_SetBits(SPI_CFG1_DSIZE_Msk, SPI_CFG1_DSIZE_Pos, dsize) |
                    whal_SetBits(SPI_CFG1_FTHLV_Msk, SPI_CFG1_FTHLV_Pos, fthlv));

    /* Set CPOL, CPHA */
    whal_Reg_Update(base, SPI_CFG2_REG,
                    SPI_CFG2_CPOL_Msk | SPI_CFG2_CPHA_Msk,
                    whal_SetBits(SPI_CFG2_CPOL_Msk, SPI_CFG2_CPOL_Pos, cpol) |
                    whal_SetBits(SPI_CFG2_CPHA_Msk, SPI_CFG2_CPHA_Pos, cpha));

    /* Endless transfer mode (TSIZE = 0) */
    whal_Reg_Update(base, SPI_CR2_REG, SPI_CR2_TSIZE_Msk, 0);

    /* Enable SPI */
    whal_Reg_Update(base, SPI_CR1_REG, SPI_CR1_SPE_Msk,
                    whal_SetBits(SPI_CR1_SPE_Msk, SPI_CR1_SPE_Pos, 1));

    /* Start master transfer */
    whal_Reg_Update(base, SPI_CR1_REG, SPI_CR1_CSTART_Msk,
                    whal_SetBits(SPI_CR1_CSTART_Msk, SPI_CR1_CSTART_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32h5_Spi_EndCom(whal_Spi *spiDev)
{
#ifdef WHAL_CFG_STM32H5_SPI_SINGLE_INSTANCE
    size_t base = whal_Stm32h5_Spi_Dev.base;
    (void)spiDev;
#else
    size_t base;

    if (!spiDev || !spiDev->cfg)
        return WHAL_EINVAL;

    base = spiDev->base;
#endif

    /* Disable SPI */
    whal_Reg_Update(base, SPI_CR1_REG, SPI_CR1_SPE_Msk,
                    whal_SetBits(SPI_CR1_SPE_Msk, SPI_CR1_SPE_Pos, 0));

    /* Clear EOT and TXTF flags */
    whal_Reg_Write(base, SPI_IFCR_REG,
                   SPI_IFCR_EOTC_Msk | SPI_IFCR_TXTFC_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32h5_Spi_SendRecv(whal_Spi *spiDev,
                                     const void *tx, size_t txLen,
                                     void *rx, size_t rxLen)
{
    const uint8_t *txBuf = (const uint8_t *)tx;
    uint8_t *rxBuf = (uint8_t *)rx;
    size_t totalLen;
    whal_Error err;
    uint8_t wordSz;
    uint8_t frameBytes;
#ifdef WHAL_CFG_STM32H5_SPI_SINGLE_INSTANCE
    whal_Stm32h5_Spi_Cfg *cfg =
        (whal_Stm32h5_Spi_Cfg *)whal_Stm32h5_Spi_Dev.cfg;
    size_t base = whal_Stm32h5_Spi_Dev.base;
    (void)spiDev;

    if ((!tx && txLen) || (!rx && rxLen))
        return WHAL_EINVAL;
#else
    size_t base;
    whal_Stm32h5_Spi_Cfg *cfg;

    if (!spiDev || !spiDev->cfg || (!tx && txLen) || (!rx && rxLen))
        return WHAL_EINVAL;

    base = spiDev->base;
    cfg = (whal_Stm32h5_Spi_Cfg *)spiDev->cfg;
#endif
    /* Frame width drives the data-register access width. DSIZE holds
     * wordSz-1; frames occupy 1/2/4 buffer bytes held native-endian
     * (little-endian on these targets). */
    wordSz = whal_GetBits(SPI_CFG1_DSIZE_Msk, SPI_CFG1_DSIZE_Pos,
                          whal_Reg_Read(base, SPI_CFG1_REG)) + 1;
    frameBytes = (wordSz > 16) ? 4 : (wordSz > 8) ? 2 : 1;

    /* A multi-byte frame spans several buffer bytes, so each buffer must
     * hold a whole number of frames. */
    if ((txLen % frameBytes) || (rxLen % frameBytes))
        return WHAL_EINVAL;

    totalLen = txLen > rxLen ? txLen : rxLen;

    for (size_t i = 0; i < totalLen; i += frameBytes) {
        uint32_t frame = 0;
        size_t b;

        /* Wait for TXP (TX FIFO has space) */
        err = whal_Reg_ReadPoll(base, SPI_SR_REG,
                                SPI_SR_TXP_Msk, SPI_SR_TXP_Msk,
                                cfg->timeout);
        if (err)
            return err;

        /* Assemble the frame from the byte buffer (native little-endian),
         * padding with 0xFF past the end of tx. */
        for (b = 0; b < frameBytes; b++) {
            uint8_t val = (txBuf && (i + b) < txLen) ? txBuf[i + b] : 0xFF;
            frame |= (uint32_t)val << (8 * b);
        }

        /* Write with the access width matching the frame size */
        if (frameBytes == 1)
            *(volatile uint8_t *)(base + SPI_TXDR_REG) = (uint8_t)frame;
        else if (frameBytes == 2)
            *(volatile uint16_t *)(base + SPI_TXDR_REG) = (uint16_t)frame;
        else
            *(volatile uint32_t *)(base + SPI_TXDR_REG) = frame;

        /* Wait for RXP (RX FIFO has data) */
        err = whal_Reg_ReadPoll(base, SPI_SR_REG,
                                SPI_SR_RXP_Msk, SPI_SR_RXP_Msk,
                                cfg->timeout);
        if (err)
            return err;

        /* Drain at the matching width */
        if (frameBytes == 1)
            frame = *(volatile uint8_t *)(base + SPI_RXDR_REG);
        else if (frameBytes == 2)
            frame = *(volatile uint16_t *)(base + SPI_RXDR_REG);
        else
            frame = *(volatile uint32_t *)(base + SPI_RXDR_REG);

        /* Split into the byte buffer native little-endian, or discard */
        if (rxBuf && i < rxLen) {
            for (b = 0; b < frameBytes; b++)
                rxBuf[i + b] = (uint8_t)(frame >> (8 * b));
        }
    }

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32H5_SPI_DIRECT_API_MAPPING
const whal_SpiDriver whal_Stm32h5_Spi_Driver = {
    .Init = whal_Stm32h5_Spi_Init,
    .Deinit = whal_Stm32h5_Spi_Deinit,
    .StartCom = whal_Stm32h5_Spi_StartCom,
    .EndCom = whal_Stm32h5_Spi_EndCom,
    .SendRecv = whal_Stm32h5_Spi_SendRecv,
};
#endif /* !WHAL_CFG_STM32H5_SPI_DIRECT_API_MAPPING */
