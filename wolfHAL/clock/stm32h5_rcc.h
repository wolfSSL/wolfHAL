/* stm32h5_rcc.h
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

#ifndef WHAL_STM32H5_RCC_H
#define WHAL_STM32H5_RCC_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * @file stm32h5_rcc.h
 * @brief STM32H5 RCC (Reset and Clock Control) driver.
 *
 * Boards bring up the clock tree imperatively from Board_Init.
 */

#define WHAL_STM32H5_RCC_BASE            0x44020C00

#define WHAL_STM32H5_RCC_CR_REG          0x000
#define WHAL_STM32H5_RCC_CR_HSIDIV_Pos   3
#define WHAL_STM32H5_RCC_CR_HSIDIV_Msk   (WHAL_BITMASK(2) << WHAL_STM32H5_RCC_CR_HSIDIV_Pos)
#define WHAL_STM32H5_RCC_CR_PLL1ON_Msk   (1UL << 24)
#define WHAL_STM32H5_RCC_CR_PLL1RDY_Msk  (1UL << 25)
#define WHAL_STM32H5_RCC_CR_PLL1RDY_Pos  25

#define WHAL_STM32H5_RCC_CFGR1_REG       0x01C
#define WHAL_STM32H5_RCC_CFGR1_SW_Pos    0
#define WHAL_STM32H5_RCC_CFGR1_SW_Msk    (WHAL_BITMASK(2) << WHAL_STM32H5_RCC_CFGR1_SW_Pos)
#define WHAL_STM32H5_RCC_CFGR1_SWS_Pos   3
#define WHAL_STM32H5_RCC_CFGR1_SWS_Msk   (WHAL_BITMASK(2) << WHAL_STM32H5_RCC_CFGR1_SWS_Pos)

#define WHAL_STM32H5_RCC_PLL1CFGR_REG          0x028
#define WHAL_STM32H5_RCC_PLL1CFGR_PLL1SRC_Pos  0
#define WHAL_STM32H5_RCC_PLL1CFGR_PLL1SRC_Msk  (WHAL_BITMASK(2) << WHAL_STM32H5_RCC_PLL1CFGR_PLL1SRC_Pos)
#define WHAL_STM32H5_RCC_PLL1CFGR_PLL1M_Pos    8
#define WHAL_STM32H5_RCC_PLL1CFGR_PLL1M_Msk    (WHAL_BITMASK(6) << WHAL_STM32H5_RCC_PLL1CFGR_PLL1M_Pos)
#define WHAL_STM32H5_RCC_PLL1CFGR_PLL1PEN_Pos  16
#define WHAL_STM32H5_RCC_PLL1CFGR_PLL1PEN_Msk  (1UL << WHAL_STM32H5_RCC_PLL1CFGR_PLL1PEN_Pos)
#define WHAL_STM32H5_RCC_PLL1CFGR_PLL1QEN_Pos  17
#define WHAL_STM32H5_RCC_PLL1CFGR_PLL1QEN_Msk  (1UL << WHAL_STM32H5_RCC_PLL1CFGR_PLL1QEN_Pos)
#define WHAL_STM32H5_RCC_PLL1CFGR_PLL1REN_Pos  18
#define WHAL_STM32H5_RCC_PLL1CFGR_PLL1REN_Msk  (1UL << WHAL_STM32H5_RCC_PLL1CFGR_PLL1REN_Pos)

#define WHAL_STM32H5_RCC_PLL1DIVR_REG          0x034
#define WHAL_STM32H5_RCC_PLL1DIVR_PLL1N_Pos    0
#define WHAL_STM32H5_RCC_PLL1DIVR_PLL1N_Msk    (WHAL_BITMASK(9) << WHAL_STM32H5_RCC_PLL1DIVR_PLL1N_Pos)
#define WHAL_STM32H5_RCC_PLL1DIVR_PLL1P_Pos    9
#define WHAL_STM32H5_RCC_PLL1DIVR_PLL1P_Msk    (WHAL_BITMASK(7) << WHAL_STM32H5_RCC_PLL1DIVR_PLL1P_Pos)
#define WHAL_STM32H5_RCC_PLL1DIVR_PLL1Q_Pos    16
#define WHAL_STM32H5_RCC_PLL1DIVR_PLL1Q_Msk    (WHAL_BITMASK(7) << WHAL_STM32H5_RCC_PLL1DIVR_PLL1Q_Pos)
#define WHAL_STM32H5_RCC_PLL1DIVR_PLL1R_Pos    24
#define WHAL_STM32H5_RCC_PLL1DIVR_PLL1R_Msk    (WHAL_BITMASK(7) << WHAL_STM32H5_RCC_PLL1DIVR_PLL1R_Pos)

/*
 * @brief System clock source selection (RCC_CFGR1.SW).
 */
typedef enum {
    WHAL_STM32H5_RCC_SYSCLK_SRC_HSI,
    WHAL_STM32H5_RCC_SYSCLK_SRC_CSI,
    WHAL_STM32H5_RCC_SYSCLK_SRC_HSE,
    WHAL_STM32H5_RCC_SYSCLK_SRC_PLL1,
} whal_Stm32h5_Rcc_SysClockSrc;

/*
 * @brief PLL input clock source (RCC_PLL1CFGR.PLL1SRC).
 */
typedef enum {
    WHAL_STM32H5_RCC_PLLCLK_SRC_NONE,
    WHAL_STM32H5_RCC_PLLCLK_SRC_HSI,
    WHAL_STM32H5_RCC_PLLCLK_SRC_CSI,
    WHAL_STM32H5_RCC_PLLCLK_SRC_HSE,
} whal_Stm32h5_Rcc_PllClockSrc;

/*
 * @brief PLL1 configuration parameters.
 *   VCO  = (input / m) * n
 *   PLLP = VCO / p (SYSCLK domain)
 *   PLLQ = VCO / q
 *   PLLR = VCO / r
 */
typedef struct {
    whal_Stm32h5_Rcc_PllClockSrc clkSrc;
    uint16_t n;
    uint8_t m;
    uint8_t p;
    uint8_t q;
    uint8_t r;
} whal_Stm32h5_Rcc_PllCfg;

/*
 * @brief Peripheral clock descriptor (RCC *ENR enable bit).
 */
typedef struct {
    size_t regOffset;
    size_t enableMask;
    size_t enablePos;
} whal_Stm32h5_Rcc_PeriphClk;

/*
 * @brief Cfg for EnableOsc/DisableOsc — on bit + ready bit.
 */
typedef struct {
    size_t onReg;
    size_t onMsk;
    size_t rdyReg;
    size_t rdyMsk;
    size_t rdyPos;
} whal_Stm32h5_Rcc_OscCfg;

#define WHAL_STM32H5_RCC_HSI_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL <<  0),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL <<  1), .rdyPos =  1
#define WHAL_STM32H5_RCC_CSI_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL <<  8),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL <<  9), .rdyPos =  9
#define WHAL_STM32H5_RCC_HSI48_CFG                           \
    .onReg  = 0x000, .onMsk  = (1UL << 12),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL << 13), .rdyPos = 13
#define WHAL_STM32H5_RCC_HSE_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL << 16),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL << 17), .rdyPos = 17

/*
 * @brief Turn on an oscillator and wait for its ready bit.
 *
 * @param cfg    Oscillator on/ready bit descriptor (e.g. WHAL_STM32H5_RCC_HSE_CFG).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32h5_Rcc_EnableOsc(
    const whal_Stm32h5_Rcc_OscCfg *cfg)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, cfg->onReg, cfg->onMsk, cfg->onMsk);
    do {
        whal_Reg32_Get(WHAL_STM32H5_RCC_BASE, cfg->rdyReg, cfg->rdyMsk,
                       cfg->rdyPos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/*
 * @brief Turn off an oscillator.
 *
 * @param cfg    Oscillator on/ready bit descriptor.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32h5_Rcc_DisableOsc(
    const whal_Stm32h5_Rcc_OscCfg *cfg)
{
    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, cfg->onReg, cfg->onMsk, 0);
    return WHAL_SUCCESS;
}

/*
 * @brief Configure and enable PLL1, waiting for lock.
 *
 * Turns PLL1 off, waits for !PLL1RDY, programs PLL1CFGR (SRC/M, output
 * enables) and PLL1DIVR (N/P/Q/R), then sets PLL1ON and polls PLL1RDY.
 *
 * @param cfg    PLL1 configuration.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32h5_Rcc_EnablePll1(
    const whal_Stm32h5_Rcc_PllCfg *cfg)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, WHAL_STM32H5_RCC_CR_REG,
                      WHAL_STM32H5_RCC_CR_PLL1ON_Msk, 0);
    do {
        whal_Reg32_Get(WHAL_STM32H5_RCC_BASE, WHAL_STM32H5_RCC_CR_REG,
                       WHAL_STM32H5_RCC_CR_PLL1RDY_Msk,
                       WHAL_STM32H5_RCC_CR_PLL1RDY_Pos, &rdy);
    } while (rdy);

    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, WHAL_STM32H5_RCC_PLL1CFGR_REG,
                      WHAL_STM32H5_RCC_PLL1CFGR_PLL1SRC_Msk |
                      WHAL_STM32H5_RCC_PLL1CFGR_PLL1M_Msk |
                      WHAL_STM32H5_RCC_PLL1CFGR_PLL1PEN_Msk |
                      WHAL_STM32H5_RCC_PLL1CFGR_PLL1QEN_Msk |
                      WHAL_STM32H5_RCC_PLL1CFGR_PLL1REN_Msk,
                      whal_SetBits(WHAL_STM32H5_RCC_PLL1CFGR_PLL1SRC_Msk,
                                   WHAL_STM32H5_RCC_PLL1CFGR_PLL1SRC_Pos, cfg->clkSrc) |
                      whal_SetBits(WHAL_STM32H5_RCC_PLL1CFGR_PLL1M_Msk,
                                   WHAL_STM32H5_RCC_PLL1CFGR_PLL1M_Pos,   cfg->m) |
                      whal_SetBits(WHAL_STM32H5_RCC_PLL1CFGR_PLL1PEN_Msk,
                                   WHAL_STM32H5_RCC_PLL1CFGR_PLL1PEN_Pos, 1) |
                      whal_SetBits(WHAL_STM32H5_RCC_PLL1CFGR_PLL1QEN_Msk,
                                   WHAL_STM32H5_RCC_PLL1CFGR_PLL1QEN_Pos, 1) |
                      whal_SetBits(WHAL_STM32H5_RCC_PLL1CFGR_PLL1REN_Msk,
                                   WHAL_STM32H5_RCC_PLL1CFGR_PLL1REN_Pos, 1));
    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, WHAL_STM32H5_RCC_PLL1DIVR_REG,
                      WHAL_STM32H5_RCC_PLL1DIVR_PLL1N_Msk |
                      WHAL_STM32H5_RCC_PLL1DIVR_PLL1P_Msk |
                      WHAL_STM32H5_RCC_PLL1DIVR_PLL1Q_Msk |
                      WHAL_STM32H5_RCC_PLL1DIVR_PLL1R_Msk,
                      whal_SetBits(WHAL_STM32H5_RCC_PLL1DIVR_PLL1N_Msk,
                                   WHAL_STM32H5_RCC_PLL1DIVR_PLL1N_Pos, cfg->n) |
                      whal_SetBits(WHAL_STM32H5_RCC_PLL1DIVR_PLL1P_Msk,
                                   WHAL_STM32H5_RCC_PLL1DIVR_PLL1P_Pos, cfg->p) |
                      whal_SetBits(WHAL_STM32H5_RCC_PLL1DIVR_PLL1Q_Msk,
                                   WHAL_STM32H5_RCC_PLL1DIVR_PLL1Q_Pos, cfg->q) |
                      whal_SetBits(WHAL_STM32H5_RCC_PLL1DIVR_PLL1R_Msk,
                                   WHAL_STM32H5_RCC_PLL1DIVR_PLL1R_Pos, cfg->r));

    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, WHAL_STM32H5_RCC_CR_REG,
                      WHAL_STM32H5_RCC_CR_PLL1ON_Msk,
                      WHAL_STM32H5_RCC_CR_PLL1ON_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32H5_RCC_BASE, WHAL_STM32H5_RCC_CR_REG,
                       WHAL_STM32H5_RCC_CR_PLL1RDY_Msk,
                       WHAL_STM32H5_RCC_CR_PLL1RDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/*
 * @brief Turn PLL1 off (clears RCC_CR.PLL1ON).
 *
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32h5_Rcc_DisablePll1(void)
{
    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, WHAL_STM32H5_RCC_CR_REG,
                      WHAL_STM32H5_RCC_CR_PLL1ON_Msk, 0);
    return WHAL_SUCCESS;
}

/*
 * @brief Switch SYSCLK to the requested source and wait for the switch.
 *
 * @param src    System clock source.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32h5_Rcc_SetSysClock(
    whal_Stm32h5_Rcc_SysClockSrc src)
{
    uint32_t sws;

    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, WHAL_STM32H5_RCC_CFGR1_REG,
                      WHAL_STM32H5_RCC_CFGR1_SW_Msk,
                      whal_SetBits(WHAL_STM32H5_RCC_CFGR1_SW_Msk,
                                   WHAL_STM32H5_RCC_CFGR1_SW_Pos, src));
    do {
        whal_Reg32_Get(WHAL_STM32H5_RCC_BASE, WHAL_STM32H5_RCC_CFGR1_REG,
                       WHAL_STM32H5_RCC_CFGR1_SWS_Msk,
                       WHAL_STM32H5_RCC_CFGR1_SWS_Pos, &sws);
    } while (sws != (size_t)src);
    return WHAL_SUCCESS;
}

/*
 * @brief Program the HSI divider (RCC_CR.HSIDIV).
 *
 * HSI = 64 MHz / 2^div. Encoded as 0..3 for /1, /2, /4, /8.
 *
 * @param div    HSIDIV encoded value (0..3).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32h5_Rcc_SetHsiDiv(
    uint8_t div)
{
    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, WHAL_STM32H5_RCC_CR_REG,
                      WHAL_STM32H5_RCC_CR_HSIDIV_Msk,
                      whal_SetBits(WHAL_STM32H5_RCC_CR_HSIDIV_Msk,
                                   WHAL_STM32H5_RCC_CR_HSIDIV_Pos, div));
    return WHAL_SUCCESS;
}

/*
 * @brief Set the enable bit for a peripheral clock gate.
 *
 * @param clk    Peripheral clock descriptor (RCC *ENR register + bit).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32h5_Rcc_EnablePeriphClk(
    const whal_Stm32h5_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 1));
    return WHAL_SUCCESS;
}

/*
 * @brief Clear the enable bit for a peripheral clock gate.
 *
 * @param clk    Peripheral clock descriptor (RCC *ENR register + bit).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32h5_Rcc_DisablePeriphClk(
    const whal_Stm32h5_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32H5_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 0));
    return WHAL_SUCCESS;
}

#endif /* WHAL_STM32H5_RCC_H */
