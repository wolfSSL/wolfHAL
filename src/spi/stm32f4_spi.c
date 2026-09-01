/* stm32f4_spi.c
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
#ifdef WHAL_CFG_STM32F4_SPI_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_Stm32f4_Spi_Dev singleton (possibly via platform alias macro) */
#endif
#include <wolfHAL/spi/stm32f4_spi.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * STM32F4 SPI Register Definitions
 *
 * The SPI peripheral provides full-duplex synchronous serial communication.
 * This driver configures it in master mode with software slave management
 * and 8-bit data frames.
 *
 * Key difference from STM32WB: data frame size is controlled by DFF bit
 * in CR1 (bit 11), not by DS field in CR2.
 */

/* Control Register 1 - master config, clock, enable */
#define SPI_CR1_REG  0x00
#define SPI_CR1_CPHA_Pos 0                                              /* Clock phase */
#define SPI_CR1_CPHA_Msk (1UL << SPI_CR1_CPHA_Pos)

#define SPI_CR1_CPOL_Pos 1                                              /* Clock polarity */
#define SPI_CR1_CPOL_Msk (1UL << SPI_CR1_CPOL_Pos)

#define SPI_CR1_MSTR_Pos 2                                              /* Master selection */
#define SPI_CR1_MSTR_Msk (1UL << SPI_CR1_MSTR_Pos)

#define SPI_CR1_BR_Pos   3                                              /* Baud rate prescaler */
#define SPI_CR1_BR_Msk   (WHAL_BITMASK(3) << SPI_CR1_BR_Pos)

#define SPI_CR1_SPE_Pos  6                                              /* SPI enable */
#define SPI_CR1_SPE_Msk  (1UL << SPI_CR1_SPE_Pos)

#define SPI_CR1_SSI_Pos  8                                              /* Internal slave select */
#define SPI_CR1_SSI_Msk  (1UL << SPI_CR1_SSI_Pos)

#define SPI_CR1_SSM_Pos  9                                              /* Software slave management */
#define SPI_CR1_SSM_Msk  (1UL << SPI_CR1_SSM_Pos)

#define SPI_CR1_DFF_Pos  11                                             /* Data frame format (0=8bit, 1=16bit) */
#define SPI_CR1_DFF_Msk  (1UL << SPI_CR1_DFF_Pos)

/* Status Register */
#define SPI_SR_REG  0x08
#define SPI_SR_RXNE_Pos 0                                               /* Receive buffer not empty */
#define SPI_SR_RXNE_Msk (1UL << SPI_SR_RXNE_Pos)

#define SPI_SR_TXE_Pos  1                                               /* Transmit buffer empty */
#define SPI_SR_TXE_Msk  (1UL << SPI_SR_TXE_Pos)

#define SPI_SR_BSY_Pos  7                                               /* Busy flag */
#define SPI_SR_BSY_Msk  (1UL << SPI_SR_BSY_Pos)

/* Data Register - 8/16-bit access */
#define SPI_DR_REG  0x0C

#ifdef WHAL_CFG_STM32F4_SPI_DIRECT_API_MAPPING
#define whal_Stm32f4_Spi_Init     whal_Spi_Init
#define whal_Stm32f4_Spi_Deinit   whal_Spi_Deinit
#define whal_Stm32f4_Spi_StartCom whal_Spi_StartCom
#define whal_Stm32f4_Spi_EndCom   whal_Spi_EndCom
#define whal_Stm32f4_Spi_SendRecv whal_Spi_SendRecv
#endif /* WHAL_CFG_STM32F4_SPI_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_STM32F4_SPI_SINGLE_INSTANCE
const whal_Spi whal_Stm32f4_Spi_Dev = WHAL_CFG_STM32F4_SPI_DEV;
#endif

/*
 * Calculate the baud rate prescaler index for a target baud rate.
 *
 * SPI baud rate = fPCLK / (2 ^ (BR + 1))
 *   BR=0 -> /2,  BR=1 -> /4,  BR=2 -> /8, ... BR=7 -> /256
 *
 * Returns the smallest prescaler that does not exceed the target baud.
 */
static uint32_t whal_Stm32f4_Spi_CalcBr(size_t pclk, uint32_t targetBaud)
{
    uint32_t br;

    for (br = 0; br < 7; br++) {
        if ((pclk / (2u << br)) <= targetBaud)
            return br;
    }

    return 7;
}

whal_Error whal_Stm32f4_Spi_Init(whal_Spi *spiDev)
{
#ifdef WHAL_CFG_STM32F4_SPI_SINGLE_INSTANCE
    size_t base = whal_Stm32f4_Spi_Dev.base;
    (void)spiDev;
#else
    size_t base;

    if (!spiDev || !spiDev->cfg)
        return WHAL_EINVAL;

    base = spiDev->base;
#endif

    /* Master mode with software slave management */
    whal_Reg_Update(base, SPI_CR1_REG,
                    SPI_CR1_MSTR_Msk | SPI_CR1_SSM_Msk | SPI_CR1_SSI_Msk,
                    whal_SetBits(SPI_CR1_MSTR_Msk, SPI_CR1_MSTR_Pos, 1) |
                    whal_SetBits(SPI_CR1_SSM_Msk, SPI_CR1_SSM_Pos, 1) |
                    whal_SetBits(SPI_CR1_SSI_Msk, SPI_CR1_SSI_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Spi_Deinit(whal_Spi *spiDev)
{
#ifdef WHAL_CFG_STM32F4_SPI_SINGLE_INSTANCE
    size_t base = whal_Stm32f4_Spi_Dev.base;
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

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Spi_StartCom(whal_Spi *spiDev, whal_Spi_ComCfg *comCfg)
{
    uint32_t cpol, cpha, br;
#ifdef WHAL_CFG_STM32F4_SPI_SINGLE_INSTANCE
    whal_Stm32f4_Spi_Cfg *cfg =
        (whal_Stm32f4_Spi_Cfg *)whal_Stm32f4_Spi_Dev.cfg;
    size_t base = whal_Stm32f4_Spi_Dev.base;
    (void)spiDev;

    if (!comCfg)
        return WHAL_EINVAL;
#else
    size_t base;
    whal_Stm32f4_Spi_Cfg *cfg;

    if (!spiDev || !spiDev->cfg || !comCfg)
        return WHAL_EINVAL;
#endif

    /* Only 8-bit frames supported */
    if (comCfg->wordSz != 8)
        return WHAL_EINVAL;

    if (comCfg->mode > 3 || comCfg->dataLines != 1 || comCfg->freq == 0)
        return WHAL_EINVAL;

#ifndef WHAL_CFG_STM32F4_SPI_SINGLE_INSTANCE
    base = spiDev->base;
    cfg = (whal_Stm32f4_Spi_Cfg *)spiDev->cfg;
#endif

    br = whal_Stm32f4_Spi_CalcBr(cfg->pclk, comCfg->freq);

    cpol = (comCfg->mode >> 1) & 1;
    cpha = comCfg->mode & 1;

    /* Disable SPE before reconfiguring */
    whal_Reg_Update(base, SPI_CR1_REG, SPI_CR1_SPE_Msk,
                    whal_SetBits(SPI_CR1_SPE_Msk, SPI_CR1_SPE_Pos, 0));

    /* Set mode and baud rate (DFF=0 for 8-bit) */
    whal_Reg_Update(base, SPI_CR1_REG,
                    SPI_CR1_CPOL_Msk | SPI_CR1_CPHA_Msk | SPI_CR1_BR_Msk | SPI_CR1_DFF_Msk,
                    whal_SetBits(SPI_CR1_CPOL_Msk, SPI_CR1_CPOL_Pos, cpol) |
                    whal_SetBits(SPI_CR1_CPHA_Msk, SPI_CR1_CPHA_Pos, cpha) |
                    whal_SetBits(SPI_CR1_BR_Msk, SPI_CR1_BR_Pos, br));

    /* Enable SPE */
    whal_Reg_Update(base, SPI_CR1_REG, SPI_CR1_SPE_Msk,
                    whal_SetBits(SPI_CR1_SPE_Msk, SPI_CR1_SPE_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Spi_EndCom(whal_Spi *spiDev)
{
#ifdef WHAL_CFG_STM32F4_SPI_SINGLE_INSTANCE
    size_t base = whal_Stm32f4_Spi_Dev.base;
    (void)spiDev;
#else
    size_t base;

    if (!spiDev || !spiDev->cfg)
        return WHAL_EINVAL;

    base = spiDev->base;
#endif

    /* Disable SPE */
    whal_Reg_Update(base, SPI_CR1_REG, SPI_CR1_SPE_Msk,
                    whal_SetBits(SPI_CR1_SPE_Msk, SPI_CR1_SPE_Pos, 0));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Spi_SendRecv(whal_Spi *spiDev,
                                     const void *tx, size_t txLen,
                                     void *rx, size_t rxLen)
{
    const uint8_t *txBuf = (const uint8_t *)tx;
    uint8_t *rxBuf = (uint8_t *)rx;
    size_t totalLen;
    whal_Error err;
    uint8_t txByte;
#ifdef WHAL_CFG_STM32F4_SPI_SINGLE_INSTANCE
    whal_Stm32f4_Spi_Cfg *cfg =
        (whal_Stm32f4_Spi_Cfg *)whal_Stm32f4_Spi_Dev.cfg;
    size_t base = whal_Stm32f4_Spi_Dev.base;
    (void)spiDev;

    if ((!tx && txLen) || (!rx && rxLen))
        return WHAL_EINVAL;
#else
    size_t base;
    whal_Stm32f4_Spi_Cfg *cfg;

    if (!spiDev || !spiDev->cfg || (!tx && txLen) || (!rx && rxLen))
        return WHAL_EINVAL;

    base = spiDev->base;
    cfg = (whal_Stm32f4_Spi_Cfg *)spiDev->cfg;
#endif
    totalLen = txLen > rxLen ? txLen : rxLen;

    for (size_t i = 0; i < totalLen; i++) {
        /* Wait for TX buffer empty */
        err = whal_Reg_ReadPoll(base, SPI_SR_REG,
                                SPI_SR_TXE_Msk, SPI_SR_TXE_Msk,
                                cfg->timeout);
        if (err)
            return err;

        /* Write TX data, pad with 0xFF when tx is exhausted or NULL */
        txByte = (txBuf && i < txLen) ? txBuf[i] : 0xFF;
        *(volatile uint8_t *)(base + SPI_DR_REG) = txByte;

        /* Wait for RX byte */
        err = whal_Reg_ReadPoll(base, SPI_SR_REG,
                                SPI_SR_RXNE_Msk, SPI_SR_RXNE_Msk,
                                cfg->timeout);
        if (err)
            return err;

        /* Store or discard received byte */
        if (rxBuf && i < rxLen)
            rxBuf[i] = *(volatile uint8_t *)(base + SPI_DR_REG);
        else
            (void)*(volatile uint8_t *)(base + SPI_DR_REG);
    }

    /* Wait for not busy */
    return whal_Reg_ReadPoll(base, SPI_SR_REG, SPI_SR_BSY_Msk, 0,
                             cfg->timeout);
}

#ifndef WHAL_CFG_STM32F4_SPI_DIRECT_API_MAPPING
const whal_SpiDriver whal_Stm32f4_Spi_Driver = {
    .Init = whal_Stm32f4_Spi_Init,
    .Deinit = whal_Stm32f4_Spi_Deinit,
    .StartCom = whal_Stm32f4_Spi_StartCom,
    .EndCom = whal_Stm32f4_Spi_EndCom,
    .SendRecv = whal_Stm32f4_Spi_SendRecv,
};
#endif /* !WHAL_CFG_SPI_API_MAPPING */
