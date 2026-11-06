/* stm32wba_rcc.h
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

#ifndef WHAL_STM32WBA_RCC_H
#define WHAL_STM32WBA_RCC_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * @file stm32wba_rcc.h
 * @brief STM32WBA RCC (Reset and Clock Control) driver.
 *
 * Boards bring up the clock tree imperatively from Board_Init. The RCC
 * peripheral lives at a fixed address — clock is a board-level driver
 * with no device struct, no generic API, no vtable.
 *
 * Differences from STM32WB:
 *   - No MSI oscillator
 *   - HSE = 32 MHz (HSE32)
 *   - PLL1 split across RCC_PLL1CFGR and RCC_PLL1DIVR
 *   - HPRE5 prescaler for AHB5 domain
 */

#define WHAL_STM32WBA_RCC_BASE            0x46020C00

#define WHAL_STM32WBA_RCC_CR_REG          0x000
#define WHAL_STM32WBA_RCC_CR_PLL1ON_Msk   (1UL << 24)
#define WHAL_STM32WBA_RCC_CR_PLL1RDY_Msk  (1UL << 25)
#define WHAL_STM32WBA_RCC_CR_PLL1RDY_Pos  25

#define WHAL_STM32WBA_RCC_CFGR1_REG       0x01C
#define WHAL_STM32WBA_RCC_CFGR1_SW_Pos    0
#define WHAL_STM32WBA_RCC_CFGR1_SW_Msk    (3UL << WHAL_STM32WBA_RCC_CFGR1_SW_Pos)
#define WHAL_STM32WBA_RCC_CFGR1_SWS_Pos   2
#define WHAL_STM32WBA_RCC_CFGR1_SWS_Msk   (3UL << WHAL_STM32WBA_RCC_CFGR1_SWS_Pos)

#define WHAL_STM32WBA_RCC_PLL1CFGR_REG     0x028
#define WHAL_STM32WBA_RCC_PLL1CFGR_SRC_Pos 0
#define WHAL_STM32WBA_RCC_PLL1CFGR_SRC_Msk (3UL << WHAL_STM32WBA_RCC_PLL1CFGR_SRC_Pos)
#define WHAL_STM32WBA_RCC_PLL1CFGR_RGE_Pos 2
#define WHAL_STM32WBA_RCC_PLL1CFGR_RGE_Msk (3UL << WHAL_STM32WBA_RCC_PLL1CFGR_RGE_Pos)
#define WHAL_STM32WBA_RCC_PLL1CFGR_M_Pos   8
#define WHAL_STM32WBA_RCC_PLL1CFGR_M_Msk   (7UL << WHAL_STM32WBA_RCC_PLL1CFGR_M_Pos)
#define WHAL_STM32WBA_RCC_PLL1CFGR_REN_Pos 18
#define WHAL_STM32WBA_RCC_PLL1CFGR_REN_Msk (1UL << WHAL_STM32WBA_RCC_PLL1CFGR_REN_Pos)

#define WHAL_STM32WBA_RCC_PLL1DIVR_REG    0x034
#define WHAL_STM32WBA_RCC_PLL1DIVR_N_Pos  0
#define WHAL_STM32WBA_RCC_PLL1DIVR_N_Msk  (0x1FFUL << WHAL_STM32WBA_RCC_PLL1DIVR_N_Pos)
#define WHAL_STM32WBA_RCC_PLL1DIVR_P_Pos  9
#define WHAL_STM32WBA_RCC_PLL1DIVR_P_Msk  (0x7FUL << WHAL_STM32WBA_RCC_PLL1DIVR_P_Pos)
#define WHAL_STM32WBA_RCC_PLL1DIVR_Q_Pos  16
#define WHAL_STM32WBA_RCC_PLL1DIVR_Q_Msk  (0x7FUL << WHAL_STM32WBA_RCC_PLL1DIVR_Q_Pos)
#define WHAL_STM32WBA_RCC_PLL1DIVR_R_Pos  24
#define WHAL_STM32WBA_RCC_PLL1DIVR_R_Msk  (0x7FUL << WHAL_STM32WBA_RCC_PLL1DIVR_R_Pos)

#define WHAL_STM32WBA_RCC_BDCR1_REG         0x0C8
#define WHAL_STM32WBA_RCC_BDCR1_LSI1ON_Msk  (1UL << 2)
#define WHAL_STM32WBA_RCC_BDCR1_LSI1RDY_Msk (1UL << 3)
#define WHAL_STM32WBA_RCC_BDCR1_LSI1RDY_Pos 3

#define WHAL_STM32WBA_RCC_CFGR4_REG       0x080
#define WHAL_STM32WBA_RCC_CFGR4_HPRE5_Pos 0
#define WHAL_STM32WBA_RCC_CFGR4_HPRE5_Msk (7UL << WHAL_STM32WBA_RCC_CFGR4_HPRE5_Pos)

#define WHAL_STM32WBA_RCC_CCIPR2_REG        0x0E4
#define WHAL_STM32WBA_RCC_CCIPR2_RNGSEL_Pos 12
#define WHAL_STM32WBA_RCC_CCIPR2_RNGSEL_Msk (3UL << WHAL_STM32WBA_RCC_CCIPR2_RNGSEL_Pos)

/*
 * @brief System clock source selection (RCC_CFGR1.SW).
 */
typedef enum {
    WHAL_STM32WBA_RCC_SYSCLK_SRC_HSI16 = 0,
    WHAL_STM32WBA_RCC_SYSCLK_SRC_HSE32 = 2,
    WHAL_STM32WBA_RCC_SYSCLK_SRC_PLL1  = 3,
} whal_Stm32wba_Rcc_SysClockSrc;

/*
 * @brief PLL1 input clock source selection.
 */
typedef enum {
    WHAL_STM32WBA_RCC_PLL1SRC_NONE  = 0,
    WHAL_STM32WBA_RCC_PLL1SRC_HSI16 = 2,
    WHAL_STM32WBA_RCC_PLL1SRC_HSE32 = 3,
} whal_Stm32wba_Rcc_Pll1Src;

/*
 * @brief PLL1 input reference frequency range.
 */
typedef enum {
    WHAL_STM32WBA_RCC_PLL1RGE_4_8  = 2,
    WHAL_STM32WBA_RCC_PLL1RGE_8_16 = 3,
} whal_Stm32wba_Rcc_Pll1Rge;

/*
 * @brief PLL1 configuration parameters.
 *   f_ref = f_input / (m + 1)    [4-16 MHz]
 *   f_vco = f_ref * n            [128-544 MHz]
 *   f_pll1r = f_vco / (r + 1)    [SYSCLK output, max 100 MHz]
 */
typedef struct {
    whal_Stm32wba_Rcc_Pll1Src clkSrc;
    whal_Stm32wba_Rcc_Pll1Rge rge;
    uint8_t m;
    uint16_t n;
    uint8_t r;
    uint8_t q;
    uint8_t p;
} whal_Stm32wba_Rcc_Pll1Cfg;

/*
 * @brief RNG kernel clock source selection (RCC_CCIPR2.RNGSEL).
 */
typedef enum {
    WHAL_STM32WBA_RCC_RNGSEL_LSE      = 0,
    WHAL_STM32WBA_RCC_RNGSEL_LSI      = 1,
    WHAL_STM32WBA_RCC_RNGSEL_HSI16    = 2,
    WHAL_STM32WBA_RCC_RNGSEL_PLL1Q_D2 = 3,
} whal_Stm32wba_Rcc_RngSrc;

/*
 * @brief Peripheral clock descriptor.
 */
typedef struct {
    size_t regOffset;
    size_t enableMask;
    size_t enablePos;
} whal_Stm32wba_Rcc_PeriphClk;

/*
 * @brief Cfg for EnableOsc/DisableOsc — on bit + ready bit.
 */
typedef struct {
    size_t onReg;
    size_t onMsk;
    size_t rdyReg;
    size_t rdyMsk;
    size_t rdyPos;
} whal_Stm32wba_Rcc_OscCfg;

#define WHAL_STM32WBA_RCC_HSI16_CFG                          \
    .onReg  = 0x000, .onMsk  = (1UL <<  8),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL << 10), .rdyPos = 10
#define WHAL_STM32WBA_RCC_HSE32_CFG                          \
    .onReg  = 0x000, .onMsk  = (1UL << 16),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL << 17), .rdyPos = 17

/*
 * @brief Turn on an oscillator (HSI16/HSE32) and wait for its ready bit.
 *
 * @param cfg    Oscillator on/ready bit descriptor.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wba_Rcc_EnableOsc(
    const whal_Stm32wba_Rcc_OscCfg *cfg)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, cfg->onReg, cfg->onMsk, cfg->onMsk);
    do {
        whal_Reg32_Get(WHAL_STM32WBA_RCC_BASE, cfg->rdyReg, cfg->rdyMsk,
                       cfg->rdyPos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/*
 * @brief Turn off an oscillator (HSI16/HSE32).
 *
 * @param cfg    Oscillator on/ready bit descriptor.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wba_Rcc_DisableOsc(
    const whal_Stm32wba_Rcc_OscCfg *cfg)
{
    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, cfg->onReg, cfg->onMsk, 0);
    return WHAL_SUCCESS;
}

/*
 * @brief Configure and enable PLL1, waiting for lock.
 *
 * Turns PLL1 off (waits for !PLL1RDY), programs PLL1CFGR (SRC/RGE/M plus
 * REN enable) and PLL1DIVR (N/P/Q/R), sets PLL1ON, polls PLL1RDY.
 *
 * @param cfg    PLL1 configuration.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wba_Rcc_EnablePll1(
    const whal_Stm32wba_Rcc_Pll1Cfg *cfg)
{
    uint32_t rdy;

    /* Disable PLL1 before reconfiguring; wait until off. */
    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_CR_REG,
                      WHAL_STM32WBA_RCC_CR_PLL1ON_Msk, 0);
    do {
        whal_Reg32_Get(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_CR_REG,
                       WHAL_STM32WBA_RCC_CR_PLL1RDY_Msk,
                       WHAL_STM32WBA_RCC_CR_PLL1RDY_Pos, &rdy);
    } while (rdy);

    whal_Reg32_Write(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_PLL1CFGR_REG,
                     whal_SetBits(WHAL_STM32WBA_RCC_PLL1CFGR_SRC_Msk,
                                  WHAL_STM32WBA_RCC_PLL1CFGR_SRC_Pos, cfg->clkSrc) |
                     whal_SetBits(WHAL_STM32WBA_RCC_PLL1CFGR_RGE_Msk,
                                  WHAL_STM32WBA_RCC_PLL1CFGR_RGE_Pos, cfg->rge) |
                     whal_SetBits(WHAL_STM32WBA_RCC_PLL1CFGR_M_Msk,
                                  WHAL_STM32WBA_RCC_PLL1CFGR_M_Pos,   cfg->m) |
                     whal_SetBits(WHAL_STM32WBA_RCC_PLL1CFGR_REN_Msk,
                                  WHAL_STM32WBA_RCC_PLL1CFGR_REN_Pos, 1));
    whal_Reg32_Write(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_PLL1DIVR_REG,
                     whal_SetBits(WHAL_STM32WBA_RCC_PLL1DIVR_N_Msk,
                                  WHAL_STM32WBA_RCC_PLL1DIVR_N_Pos, cfg->n) |
                     whal_SetBits(WHAL_STM32WBA_RCC_PLL1DIVR_P_Msk,
                                  WHAL_STM32WBA_RCC_PLL1DIVR_P_Pos, cfg->p) |
                     whal_SetBits(WHAL_STM32WBA_RCC_PLL1DIVR_Q_Msk,
                                  WHAL_STM32WBA_RCC_PLL1DIVR_Q_Pos, cfg->q) |
                     whal_SetBits(WHAL_STM32WBA_RCC_PLL1DIVR_R_Msk,
                                  WHAL_STM32WBA_RCC_PLL1DIVR_R_Pos, cfg->r));

    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_CR_REG,
                      WHAL_STM32WBA_RCC_CR_PLL1ON_Msk,
                      WHAL_STM32WBA_RCC_CR_PLL1ON_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_CR_REG,
                       WHAL_STM32WBA_RCC_CR_PLL1RDY_Msk,
                       WHAL_STM32WBA_RCC_CR_PLL1RDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/*
 * @brief Turn PLL1 off (clears RCC_CR.PLL1ON).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wba_Rcc_DisablePll1(void)
{
    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_CR_REG,
                      WHAL_STM32WBA_RCC_CR_PLL1ON_Msk, 0);
    return WHAL_SUCCESS;
}

/*
 * @brief Enable LSI1 (32 kHz internal RC) and wait for ready.
 *
 * Sets BDCR1.LSI1ON and polls BDCR1.LSI1RDY.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wba_Rcc_EnableLsi(void)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_BDCR1_REG,
                      WHAL_STM32WBA_RCC_BDCR1_LSI1ON_Msk,
                      WHAL_STM32WBA_RCC_BDCR1_LSI1ON_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_BDCR1_REG,
                       WHAL_STM32WBA_RCC_BDCR1_LSI1RDY_Msk,
                       WHAL_STM32WBA_RCC_BDCR1_LSI1RDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/*
 * @brief Disable LSI1 (clears BDCR1.LSI1ON).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wba_Rcc_DisableLsi(void)
{
    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_BDCR1_REG,
                      WHAL_STM32WBA_RCC_BDCR1_LSI1ON_Msk, 0);
    return WHAL_SUCCESS;
}

/*
 * @brief Switch SYSCLK to the requested source and wait for the switch.
 *
 * @param src    System clock source.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wba_Rcc_SetSysClock(
    whal_Stm32wba_Rcc_SysClockSrc src)
{
    uint32_t sws;

    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_CFGR1_REG,
                      WHAL_STM32WBA_RCC_CFGR1_SW_Msk,
                      whal_SetBits(WHAL_STM32WBA_RCC_CFGR1_SW_Msk,
                                   WHAL_STM32WBA_RCC_CFGR1_SW_Pos, src));
    do {
        whal_Reg32_Get(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_CFGR1_REG,
                       WHAL_STM32WBA_RCC_CFGR1_SWS_Msk,
                       WHAL_STM32WBA_RCC_CFGR1_SWS_Pos, &sws);
    } while (sws != (size_t)src);
    return WHAL_SUCCESS;
}

/*
 * @brief Select the RNG kernel clock source (RCC_CCIPR2.RNGSEL).
 *
 * @param src    RNG kernel clock source.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wba_Rcc_SetRngClockSrc(
    whal_Stm32wba_Rcc_RngSrc src)
{
    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_CCIPR2_REG,
                      WHAL_STM32WBA_RCC_CCIPR2_RNGSEL_Msk,
                      whal_SetBits(WHAL_STM32WBA_RCC_CCIPR2_RNGSEL_Msk,
                                   WHAL_STM32WBA_RCC_CCIPR2_RNGSEL_Pos,
                                   src));
    return WHAL_SUCCESS;
}

/*
 * @brief Program the AHB5 domain prescaler (RCC_CFGR4.HPRE5).
 *
 * @param hpre5  HPRE5 encoded divider value.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wba_Rcc_SetHpre5(uint8_t hpre5)
{
    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, WHAL_STM32WBA_RCC_CFGR4_REG,
                      WHAL_STM32WBA_RCC_CFGR4_HPRE5_Msk,
                      whal_SetBits(WHAL_STM32WBA_RCC_CFGR4_HPRE5_Msk,
                                   WHAL_STM32WBA_RCC_CFGR4_HPRE5_Pos, hpre5));
    return WHAL_SUCCESS;
}

/*
 * @brief Set the enable bit for a peripheral clock gate.
 *
 * @param clk    Peripheral clock descriptor (RCC *ENR register + bit).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wba_Rcc_EnablePeriphClk(
    const whal_Stm32wba_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, clk->regOffset, clk->enableMask,
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
static inline whal_Error whal_Stm32wba_Rcc_DisablePeriphClk(
    const whal_Stm32wba_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32WBA_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 0));
    return WHAL_SUCCESS;
}

#endif /* WHAL_STM32WBA_RCC_H */
