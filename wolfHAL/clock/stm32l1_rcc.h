/* stm32l1_rcc.h
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

#ifndef WHAL_STM32L1_RCC_H
#define WHAL_STM32L1_RCC_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * @file stm32l1_rcc.h
 * @brief STM32L1 RCC (Reset and Clock Control) driver.
 *
 * Boards bring up the clock tree imperatively from Board_Init.
 *
 * Clock sources:
 *   MSI = multispeed internal (default after reset)
 *   HSI = 16 MHz internal RC
 *   HSE = 1-24 MHz external
 *   PLL = HSI or HSE * PLLMUL / PLLDIV (max 32 MHz)
 */

#define WHAL_STM32L1_RCC_BASE            0x40023800

#define WHAL_STM32L1_RCC_CR_REG          0x00
#define WHAL_STM32L1_RCC_CR_PLLON_Msk    (1UL << 24)
#define WHAL_STM32L1_RCC_CR_PLLRDY_Msk   (1UL << 25)
#define WHAL_STM32L1_RCC_CR_PLLRDY_Pos   25

#define WHAL_STM32L1_RCC_CFGR_REG        0x08
#define WHAL_STM32L1_RCC_CFGR_SW_Pos     0
#define WHAL_STM32L1_RCC_CFGR_SW_Msk     (WHAL_BITMASK(2) << WHAL_STM32L1_RCC_CFGR_SW_Pos)
#define WHAL_STM32L1_RCC_CFGR_SWS_Pos    2
#define WHAL_STM32L1_RCC_CFGR_SWS_Msk    (WHAL_BITMASK(2) << WHAL_STM32L1_RCC_CFGR_SWS_Pos)
#define WHAL_STM32L1_RCC_CFGR_PLLSRC_Pos 16
#define WHAL_STM32L1_RCC_CFGR_PLLSRC_Msk (1UL << WHAL_STM32L1_RCC_CFGR_PLLSRC_Pos)
#define WHAL_STM32L1_RCC_CFGR_PLLMUL_Pos 18
#define WHAL_STM32L1_RCC_CFGR_PLLMUL_Msk (WHAL_BITMASK(4) << WHAL_STM32L1_RCC_CFGR_PLLMUL_Pos)
#define WHAL_STM32L1_RCC_CFGR_PLLDIV_Pos 22
#define WHAL_STM32L1_RCC_CFGR_PLLDIV_Msk (WHAL_BITMASK(2) << WHAL_STM32L1_RCC_CFGR_PLLDIV_Pos)

/*
 * @brief System clock source selection (RCC_CFGR.SW).
 */
typedef enum {
    WHAL_STM32L1_RCC_SYSCLK_SRC_MSI,
    WHAL_STM32L1_RCC_SYSCLK_SRC_HSI,
    WHAL_STM32L1_RCC_SYSCLK_SRC_HSE,
    WHAL_STM32L1_RCC_SYSCLK_SRC_PLL,
} whal_Stm32l1_Rcc_SysClockSrc;

/*
 * @brief PLL input selection.
 */
typedef enum {
    WHAL_STM32L1_RCC_PLLSRC_HSI,
    WHAL_STM32L1_RCC_PLLSRC_HSE,
} whal_Stm32l1_Rcc_PllClockSrc;

/*
 * @brief PLL multiplication factor (RCC_CFGR.PLLMUL).
 */
typedef enum {
    WHAL_STM32L1_RCC_PLLMUL_3  = 0,
    WHAL_STM32L1_RCC_PLLMUL_4  = 1,
    WHAL_STM32L1_RCC_PLLMUL_6  = 2,
    WHAL_STM32L1_RCC_PLLMUL_8  = 3,
    WHAL_STM32L1_RCC_PLLMUL_12 = 4,
    WHAL_STM32L1_RCC_PLLMUL_16 = 5,
    WHAL_STM32L1_RCC_PLLMUL_24 = 6,
    WHAL_STM32L1_RCC_PLLMUL_32 = 7,
    WHAL_STM32L1_RCC_PLLMUL_48 = 8,
} whal_Stm32l1_Rcc_PllMul;

/*
 * @brief PLL output division factor (RCC_CFGR.PLLDIV).
 */
typedef enum {
    WHAL_STM32L1_RCC_PLLDIV_2 = 1,
    WHAL_STM32L1_RCC_PLLDIV_3 = 2,
    WHAL_STM32L1_RCC_PLLDIV_4 = 3,
} whal_Stm32l1_Rcc_PllDiv;

/*
 * @brief PLL configuration parameters.
 */
typedef struct {
    whal_Stm32l1_Rcc_PllClockSrc clkSrc;
    whal_Stm32l1_Rcc_PllMul pllmul;
    whal_Stm32l1_Rcc_PllDiv plldiv;
} whal_Stm32l1_Rcc_PllCfg;

/*
 * @brief Peripheral clock descriptor.
 */
typedef struct {
    size_t regOffset;
    size_t enableMask;
    size_t enablePos;
} whal_Stm32l1_Rcc_PeriphClk;

/*
 * @brief Cfg for EnableOsc/DisableOsc — on bit + ready bit.
 */
typedef struct {
    size_t onReg;
    size_t onMsk;
    size_t rdyReg;
    size_t rdyMsk;
    size_t rdyPos;
} whal_Stm32l1_Rcc_OscCfg;

#define WHAL_STM32L1_RCC_HSI_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL <<  0),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL <<  1), .rdyPos =  1
#define WHAL_STM32L1_RCC_HSE_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL << 16),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL << 17), .rdyPos = 17

/*
 * @brief Turn on an oscillator (HSI/HSE) and wait for its ready bit.
 *
 * @param cfg    Oscillator on/ready bit descriptor.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32l1_Rcc_EnableOsc(
    const whal_Stm32l1_Rcc_OscCfg *cfg)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, cfg->onReg, cfg->onMsk, cfg->onMsk);
    do {
        whal_Reg32_Get(WHAL_STM32L1_RCC_BASE, cfg->rdyReg, cfg->rdyMsk,
                       cfg->rdyPos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/*
 * @brief Turn off an oscillator (HSI/HSE).
 *
 * @param cfg    Oscillator on/ready bit descriptor.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32l1_Rcc_DisableOsc(
    const whal_Stm32l1_Rcc_OscCfg *cfg)
{
    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, cfg->onReg, cfg->onMsk, 0);
    return WHAL_SUCCESS;
}

/*
 * @brief Configure and enable the PLL, waiting for lock.
 *
 * Turns the PLL off (waits for !PLLRDY), programs PLLSRC, PLLMUL, PLLDIV,
 * sets PLLON, and polls PLLRDY. Requires PWR VOS range high enough for the
 * target frequency — see stm32l1_pwr.h.
 *
 * @param cfg    PLL configuration.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32l1_Rcc_EnablePll(
    const whal_Stm32l1_Rcc_PllCfg *cfg)
{
    uint32_t rdy;
    uint32_t pllsrc;

    /* Disable PLL before reconfiguring; wait until it's actually off. */
    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, WHAL_STM32L1_RCC_CR_REG,
                      WHAL_STM32L1_RCC_CR_PLLON_Msk, 0);
    do {
        whal_Reg32_Get(WHAL_STM32L1_RCC_BASE, WHAL_STM32L1_RCC_CR_REG,
                       WHAL_STM32L1_RCC_CR_PLLRDY_Msk,
                       WHAL_STM32L1_RCC_CR_PLLRDY_Pos, &rdy);
    } while (rdy);

    pllsrc = (cfg->clkSrc == WHAL_STM32L1_RCC_PLLSRC_HSE) ? 1 : 0;
    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, WHAL_STM32L1_RCC_CFGR_REG,
                      WHAL_STM32L1_RCC_CFGR_PLLSRC_Msk,
                      whal_SetBits(WHAL_STM32L1_RCC_CFGR_PLLSRC_Msk,
                                   WHAL_STM32L1_RCC_CFGR_PLLSRC_Pos,
                                   pllsrc));
    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, WHAL_STM32L1_RCC_CFGR_REG,
                      WHAL_STM32L1_RCC_CFGR_PLLMUL_Msk,
                      whal_SetBits(WHAL_STM32L1_RCC_CFGR_PLLMUL_Msk,
                                   WHAL_STM32L1_RCC_CFGR_PLLMUL_Pos,
                                   cfg->pllmul));
    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, WHAL_STM32L1_RCC_CFGR_REG,
                      WHAL_STM32L1_RCC_CFGR_PLLDIV_Msk,
                      whal_SetBits(WHAL_STM32L1_RCC_CFGR_PLLDIV_Msk,
                                   WHAL_STM32L1_RCC_CFGR_PLLDIV_Pos,
                                   cfg->plldiv));

    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, WHAL_STM32L1_RCC_CR_REG,
                      WHAL_STM32L1_RCC_CR_PLLON_Msk,
                      WHAL_STM32L1_RCC_CR_PLLON_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32L1_RCC_BASE, WHAL_STM32L1_RCC_CR_REG,
                       WHAL_STM32L1_RCC_CR_PLLRDY_Msk,
                       WHAL_STM32L1_RCC_CR_PLLRDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/*
 * @brief Turn the PLL off (clears RCC_CR.PLLON).
 *
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32l1_Rcc_DisablePll(void)
{
    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, WHAL_STM32L1_RCC_CR_REG,
                      WHAL_STM32L1_RCC_CR_PLLON_Msk, 0);
    return WHAL_SUCCESS;
}

/*
 * @brief Switch SYSCLK to the requested source and wait for the switch.
 *
 * @param src    System clock source.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32l1_Rcc_SetSysClock(
    whal_Stm32l1_Rcc_SysClockSrc src)
{
    uint32_t sws;

    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, WHAL_STM32L1_RCC_CFGR_REG,
                      WHAL_STM32L1_RCC_CFGR_SW_Msk,
                      whal_SetBits(WHAL_STM32L1_RCC_CFGR_SW_Msk,
                                   WHAL_STM32L1_RCC_CFGR_SW_Pos, src));
    do {
        whal_Reg32_Get(WHAL_STM32L1_RCC_BASE, WHAL_STM32L1_RCC_CFGR_REG,
                       WHAL_STM32L1_RCC_CFGR_SWS_Msk,
                       WHAL_STM32L1_RCC_CFGR_SWS_Pos, &sws);
    } while (sws != (size_t)src);
    return WHAL_SUCCESS;
}

/*
 * @brief Set the enable bit for a peripheral clock gate.
 *
 * @param clk    Peripheral clock descriptor (RCC *ENR register + bit).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32l1_Rcc_EnablePeriphClk(
    const whal_Stm32l1_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, clk->regOffset, clk->enableMask,
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
static inline whal_Error whal_Stm32l1_Rcc_DisablePeriphClk(
    const whal_Stm32l1_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32L1_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 0));
    return WHAL_SUCCESS;
}

#endif /* WHAL_STM32L1_RCC_H */
