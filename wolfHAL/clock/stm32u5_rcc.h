/* stm32u5_rcc.h
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

#ifndef WHAL_STM32U5_RCC_H
#define WHAL_STM32U5_RCC_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/**
 * @file stm32u5_rcc.h
 * @brief STM32U5 RCC (Reset and Clock Control) driver.
 *
 * Boards bring up the clock tree imperatively from Board_Init. The RCC
 * peripheral lives at a fixed address — clock is a board-level driver
 * with no device struct, no generic API, no vtable.
 *
 * Differences from STM32WBA:
 *   - PLL1M is 4 bits (bits 11:8) — selects /1 to /16
 *   - PLL1CFGR adds PLL1MBOOST (bits 15:12) and PLL1FRACEN (bit 4)
 *   - Voltage scaling has four ranges (VOS[1:0]: 0=Range 4, 1=Range 3,
 *     2=Range 2, 3=Range 1). EPOD booster must be enabled for SYSCLK
 *     above 55 MHz in Range 1.
 *   - MSI oscillator available (MSIS for sysclk, MSIK for kernel).
 */

#define WHAL_STM32U5_RCC_BASE              0x46020C00

#define WHAL_STM32U5_RCC_CR_REG            0x000
#define WHAL_STM32U5_RCC_CR_MSISON_Msk     (1UL << 0)
#define WHAL_STM32U5_RCC_CR_MSISRDY_Msk    (1UL << 2)
#define WHAL_STM32U5_RCC_CR_MSISRDY_Pos    2
#define WHAL_STM32U5_RCC_CR_HSION_Msk      (1UL << 8)
#define WHAL_STM32U5_RCC_CR_HSIRDY_Msk     (1UL << 10)
#define WHAL_STM32U5_RCC_CR_HSIRDY_Pos     10
#define WHAL_STM32U5_RCC_CR_HSEON_Msk      (1UL << 16)
#define WHAL_STM32U5_RCC_CR_HSERDY_Msk     (1UL << 17)
#define WHAL_STM32U5_RCC_CR_HSERDY_Pos     17
#define WHAL_STM32U5_RCC_CR_PLL1ON_Msk     (1UL << 24)
#define WHAL_STM32U5_RCC_CR_PLL1RDY_Msk    (1UL << 25)
#define WHAL_STM32U5_RCC_CR_PLL1RDY_Pos    25

#define WHAL_STM32U5_RCC_CFGR1_REG         0x01C
#define WHAL_STM32U5_RCC_CFGR1_SW_Pos      0
#define WHAL_STM32U5_RCC_CFGR1_SW_Msk      (3UL << WHAL_STM32U5_RCC_CFGR1_SW_Pos)
#define WHAL_STM32U5_RCC_CFGR1_SWS_Pos     2
#define WHAL_STM32U5_RCC_CFGR1_SWS_Msk     (3UL << WHAL_STM32U5_RCC_CFGR1_SWS_Pos)

/* PLL1CFGR bit layout differs from WBA: PLL1M is 4 bits wide,
 * PLL1MBOOST sits at 15:12, PLL1FRACEN at bit 4. */
#define WHAL_STM32U5_RCC_PLL1CFGR_REG          0x028
#define WHAL_STM32U5_RCC_PLL1CFGR_SRC_Pos      0
#define WHAL_STM32U5_RCC_PLL1CFGR_SRC_Msk      (3UL << WHAL_STM32U5_RCC_PLL1CFGR_SRC_Pos)
#define WHAL_STM32U5_RCC_PLL1CFGR_RGE_Pos      2
#define WHAL_STM32U5_RCC_PLL1CFGR_RGE_Msk      (3UL << WHAL_STM32U5_RCC_PLL1CFGR_RGE_Pos)
#define WHAL_STM32U5_RCC_PLL1CFGR_FRACEN_Pos   4
#define WHAL_STM32U5_RCC_PLL1CFGR_FRACEN_Msk   (1UL << WHAL_STM32U5_RCC_PLL1CFGR_FRACEN_Pos)
#define WHAL_STM32U5_RCC_PLL1CFGR_M_Pos        8
#define WHAL_STM32U5_RCC_PLL1CFGR_M_Msk        (0xFUL << WHAL_STM32U5_RCC_PLL1CFGR_M_Pos)
#define WHAL_STM32U5_RCC_PLL1CFGR_MBOOST_Pos   12
#define WHAL_STM32U5_RCC_PLL1CFGR_MBOOST_Msk   (0xFUL << WHAL_STM32U5_RCC_PLL1CFGR_MBOOST_Pos)
#define WHAL_STM32U5_RCC_PLL1CFGR_PEN_Pos      16
#define WHAL_STM32U5_RCC_PLL1CFGR_PEN_Msk      (1UL << WHAL_STM32U5_RCC_PLL1CFGR_PEN_Pos)
#define WHAL_STM32U5_RCC_PLL1CFGR_QEN_Pos      17
#define WHAL_STM32U5_RCC_PLL1CFGR_QEN_Msk      (1UL << WHAL_STM32U5_RCC_PLL1CFGR_QEN_Pos)
#define WHAL_STM32U5_RCC_PLL1CFGR_REN_Pos      18
#define WHAL_STM32U5_RCC_PLL1CFGR_REN_Msk      (1UL << WHAL_STM32U5_RCC_PLL1CFGR_REN_Pos)

#define WHAL_STM32U5_RCC_PLL1DIVR_REG     0x034
#define WHAL_STM32U5_RCC_PLL1DIVR_N_Pos   0
#define WHAL_STM32U5_RCC_PLL1DIVR_N_Msk   (0x1FFUL << WHAL_STM32U5_RCC_PLL1DIVR_N_Pos)
#define WHAL_STM32U5_RCC_PLL1DIVR_P_Pos   9
#define WHAL_STM32U5_RCC_PLL1DIVR_P_Msk   (0x7FUL << WHAL_STM32U5_RCC_PLL1DIVR_P_Pos)
#define WHAL_STM32U5_RCC_PLL1DIVR_Q_Pos   16
#define WHAL_STM32U5_RCC_PLL1DIVR_Q_Msk   (0x7FUL << WHAL_STM32U5_RCC_PLL1DIVR_Q_Pos)
#define WHAL_STM32U5_RCC_PLL1DIVR_R_Pos   24
#define WHAL_STM32U5_RCC_PLL1DIVR_R_Msk   (0x7FUL << WHAL_STM32U5_RCC_PLL1DIVR_R_Pos)

/* BDCR1 holds LSE/LSI/RTC clocking */
#define WHAL_STM32U5_RCC_BDCR_REG          0x0F0
#define WHAL_STM32U5_RCC_BDCR_LSION_Msk    (1UL << 26)
#define WHAL_STM32U5_RCC_BDCR_LSIRDY_Msk   (1UL << 27)
#define WHAL_STM32U5_RCC_BDCR_LSIRDY_Pos   27

#define WHAL_STM32U5_RCC_CCIPR2_REG        0x0E4
#define WHAL_STM32U5_RCC_CCIPR2_RNGSEL_Pos 12
#define WHAL_STM32U5_RCC_CCIPR2_RNGSEL_Msk (3UL << WHAL_STM32U5_RCC_CCIPR2_RNGSEL_Pos)

/* PWR registers used during clock bring-up */
#define WHAL_STM32U5_PWR_BASE              0x46020800
#define WHAL_STM32U5_PWR_VOSR_REG          0x00C
#define WHAL_STM32U5_PWR_VOSR_VOSRDY_Msk   (1UL << 15)
#define WHAL_STM32U5_PWR_VOSR_VOSRDY_Pos   15
#define WHAL_STM32U5_PWR_VOSR_BOOSTRDY_Msk (1UL << 14)
#define WHAL_STM32U5_PWR_VOSR_BOOSTRDY_Pos 14
#define WHAL_STM32U5_PWR_VOSR_VOS_Pos      16
#define WHAL_STM32U5_PWR_VOSR_VOS_Msk      (3UL << WHAL_STM32U5_PWR_VOSR_VOS_Pos)
#define WHAL_STM32U5_PWR_VOSR_BOOSTEN_Pos  18
#define WHAL_STM32U5_PWR_VOSR_BOOSTEN_Msk  (1UL << 18)

/**
 * @brief System clock source selection (RCC_CFGR1.SW).
 */
typedef enum {
    WHAL_STM32U5_RCC_SYSCLK_SRC_MSIS  = 0,
    WHAL_STM32U5_RCC_SYSCLK_SRC_HSI16 = 1,
    WHAL_STM32U5_RCC_SYSCLK_SRC_HSE   = 2,
    WHAL_STM32U5_RCC_SYSCLK_SRC_PLL1  = 3,
} whal_Stm32u5_Rcc_SysClockSrc;

/**
 * @brief PLL1 input clock source selection.
 */
typedef enum {
    WHAL_STM32U5_RCC_PLL1SRC_NONE  = 0,
    WHAL_STM32U5_RCC_PLL1SRC_MSIS  = 1,
    WHAL_STM32U5_RCC_PLL1SRC_HSI16 = 2,
    WHAL_STM32U5_RCC_PLL1SRC_HSE   = 3,
} whal_Stm32u5_Rcc_Pll1Src;

/**
 * @brief PLL1 input reference frequency range.
 */
typedef enum {
    WHAL_STM32U5_RCC_PLL1RGE_4_8   = 0,
    WHAL_STM32U5_RCC_PLL1RGE_8_16  = 3,
} whal_Stm32u5_Rcc_Pll1Rge;

/**
 * @brief Voltage scaling range (PWR_VOSR.VOS[1:0]). Range 1 is required for
 *        SYSCLK above 24 MHz; the EPOD booster is required above 55 MHz.
 */
typedef enum {
    WHAL_STM32U5_PWR_VOS_RANGE_4 = 0,
    WHAL_STM32U5_PWR_VOS_RANGE_3 = 1,
    WHAL_STM32U5_PWR_VOS_RANGE_2 = 2,
    WHAL_STM32U5_PWR_VOS_RANGE_1 = 3,
} whal_Stm32u5_Pwr_VosRange;

/**
 * @brief PLL1 configuration parameters.
 *   f_ref = f_input / m       [4-16 MHz]
 *   f_vco = f_ref * n         [128-544 MHz]
 *   f_pll1r = f_vco / r       [SYSCLK output, max 160 MHz]
 *
 * `m`, `n`, `r`, `q`, `p` are the actual divider/multiplier values; the
 * driver encodes them as field = value - 1 when writing the register
 * (matching ST HAL conventions). Valid ranges: m=1..16, n=4..512,
 * r/q/p=1..128.
 *
 * `mboost` is the EPOD prescaler field — written raw; 0 bypasses the
 * prescaler.
 */
typedef struct {
    whal_Stm32u5_Rcc_Pll1Src clkSrc;
    whal_Stm32u5_Rcc_Pll1Rge rge;
    uint8_t m;       /**< PLL1M divider, 1..16 */
    uint8_t mboost;  /**< PLL1MBOOST[3:0] — 0 bypasses EPOD prescaler */
    uint16_t n;      /**< PLL1N multiplier, 4..512 */
    uint8_t r;       /**< PLL1R divider, 1..128 (SYSCLK output) */
    uint8_t q;       /**< PLL1Q divider, 1..128 */
    uint8_t p;       /**< PLL1P divider, 1..128 */
} whal_Stm32u5_Rcc_Pll1Cfg;

/**
 * @brief RNG kernel clock source selection (RCC_CCIPR2.RNGSEL).
 */
typedef enum {
    WHAL_STM32U5_RCC_RNGSEL_HSI48      = 0,
    WHAL_STM32U5_RCC_RNGSEL_HSI48_DIV2 = 1,
    WHAL_STM32U5_RCC_RNGSEL_HSI16      = 2,
} whal_Stm32u5_Rcc_RngSrc;

/**
 * @brief Peripheral clock descriptor.
 */
typedef struct {
    size_t regOffset;
    size_t enableMask;
    size_t enablePos;
} whal_Stm32u5_Rcc_PeriphClk;

/**
 * @brief Cfg for EnableOsc/DisableOsc — on bit + ready bit.
 */
typedef struct {
    size_t onReg;
    size_t onMsk;
    size_t rdyReg;
    size_t rdyMsk;
    size_t rdyPos;
} whal_Stm32u5_Rcc_OscCfg;

#define WHAL_STM32U5_RCC_HSI16_CFG                           \
    .onReg  = 0x000, .onMsk  = (1UL <<  8),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL << 10), .rdyPos = 10
#define WHAL_STM32U5_RCC_HSE_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL << 16),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL << 17), .rdyPos = 17

/**
 * @brief Turn on an oscillator (HSI16/HSE) and wait for its ready bit.
 *
 * @param cfg Oscillator on/ready bit descriptor.
 */
static inline whal_Error whal_Stm32u5_Rcc_EnableOsc(
    const whal_Stm32u5_Rcc_OscCfg *cfg)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, cfg->onReg, cfg->onMsk, cfg->onMsk);
    do {
        whal_Reg32_Get(WHAL_STM32U5_RCC_BASE, cfg->rdyReg, cfg->rdyMsk,
                       cfg->rdyPos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/**
 * @brief Turn off an oscillator.
 */
static inline whal_Error whal_Stm32u5_Rcc_DisableOsc(
    const whal_Stm32u5_Rcc_OscCfg *cfg)
{
    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, cfg->onReg, cfg->onMsk, 0);
    return WHAL_SUCCESS;
}

/**
 * @brief Configure and enable PLL1, waiting for lock.
 */
static inline whal_Error whal_Stm32u5_Rcc_EnablePll1(
    const whal_Stm32u5_Rcc_Pll1Cfg *cfg)
{
    uint32_t rdy;

    /* Disable PLL1 before reconfiguring; wait until off. */
    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_CR_REG,
                      WHAL_STM32U5_RCC_CR_PLL1ON_Msk, 0);
    do {
        whal_Reg32_Get(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_CR_REG,
                       WHAL_STM32U5_RCC_CR_PLL1RDY_Msk,
                       WHAL_STM32U5_RCC_CR_PLL1RDY_Pos, &rdy);
    } while (rdy);

    whal_Reg32_Write(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_PLL1CFGR_REG,
                     whal_SetBits(WHAL_STM32U5_RCC_PLL1CFGR_SRC_Msk,
                                  WHAL_STM32U5_RCC_PLL1CFGR_SRC_Pos, cfg->clkSrc) |
                     whal_SetBits(WHAL_STM32U5_RCC_PLL1CFGR_RGE_Msk,
                                  WHAL_STM32U5_RCC_PLL1CFGR_RGE_Pos, cfg->rge) |
                     whal_SetBits(WHAL_STM32U5_RCC_PLL1CFGR_M_Msk,
                                  WHAL_STM32U5_RCC_PLL1CFGR_M_Pos,   cfg->m - 1) |
                     whal_SetBits(WHAL_STM32U5_RCC_PLL1CFGR_MBOOST_Msk,
                                  WHAL_STM32U5_RCC_PLL1CFGR_MBOOST_Pos,
                                  cfg->mboost) |
                     whal_SetBits(WHAL_STM32U5_RCC_PLL1CFGR_REN_Msk,
                                  WHAL_STM32U5_RCC_PLL1CFGR_REN_Pos, 1));
    whal_Reg32_Write(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_PLL1DIVR_REG,
                     whal_SetBits(WHAL_STM32U5_RCC_PLL1DIVR_N_Msk,
                                  WHAL_STM32U5_RCC_PLL1DIVR_N_Pos, cfg->n - 1) |
                     whal_SetBits(WHAL_STM32U5_RCC_PLL1DIVR_P_Msk,
                                  WHAL_STM32U5_RCC_PLL1DIVR_P_Pos, cfg->p - 1) |
                     whal_SetBits(WHAL_STM32U5_RCC_PLL1DIVR_Q_Msk,
                                  WHAL_STM32U5_RCC_PLL1DIVR_Q_Pos, cfg->q - 1) |
                     whal_SetBits(WHAL_STM32U5_RCC_PLL1DIVR_R_Msk,
                                  WHAL_STM32U5_RCC_PLL1DIVR_R_Pos, cfg->r - 1));

    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_CR_REG,
                      WHAL_STM32U5_RCC_CR_PLL1ON_Msk,
                      WHAL_STM32U5_RCC_CR_PLL1ON_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_CR_REG,
                       WHAL_STM32U5_RCC_CR_PLL1RDY_Msk,
                       WHAL_STM32U5_RCC_CR_PLL1RDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/**
 * @brief Turn PLL1 off (clears RCC_CR.PLL1ON).
 */
static inline whal_Error whal_Stm32u5_Rcc_DisablePll1(void)
{
    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_CR_REG,
                      WHAL_STM32U5_RCC_CR_PLL1ON_Msk, 0);
    return WHAL_SUCCESS;
}

/**
 * @brief Enable LSI (32 kHz internal RC) and wait for ready.
 */
static inline whal_Error whal_Stm32u5_Rcc_EnableLsi(void)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_BDCR_REG,
                      WHAL_STM32U5_RCC_BDCR_LSION_Msk,
                      WHAL_STM32U5_RCC_BDCR_LSION_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_BDCR_REG,
                       WHAL_STM32U5_RCC_BDCR_LSIRDY_Msk,
                       WHAL_STM32U5_RCC_BDCR_LSIRDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/**
 * @brief Disable LSI.
 */
static inline whal_Error whal_Stm32u5_Rcc_DisableLsi(void)
{
    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_BDCR_REG,
                      WHAL_STM32U5_RCC_BDCR_LSION_Msk, 0);
    return WHAL_SUCCESS;
}

/**
 * @brief Switch SYSCLK to the requested source and wait for the switch.
 */
static inline whal_Error whal_Stm32u5_Rcc_SetSysClock(
    whal_Stm32u5_Rcc_SysClockSrc src)
{
    uint32_t sws;

    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_CFGR1_REG,
                      WHAL_STM32U5_RCC_CFGR1_SW_Msk,
                      whal_SetBits(WHAL_STM32U5_RCC_CFGR1_SW_Msk,
                                   WHAL_STM32U5_RCC_CFGR1_SW_Pos, src));
    do {
        whal_Reg32_Get(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_CFGR1_REG,
                       WHAL_STM32U5_RCC_CFGR1_SWS_Msk,
                       WHAL_STM32U5_RCC_CFGR1_SWS_Pos, &sws);
    } while (sws != (size_t)src);
    return WHAL_SUCCESS;
}

/**
 * @brief Select the RNG kernel clock source (RCC_CCIPR2.RNGSEL).
 */
static inline whal_Error whal_Stm32u5_Rcc_SetRngClockSrc(
    whal_Stm32u5_Rcc_RngSrc src)
{
    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, WHAL_STM32U5_RCC_CCIPR2_REG,
                      WHAL_STM32U5_RCC_CCIPR2_RNGSEL_Msk,
                      whal_SetBits(WHAL_STM32U5_RCC_CCIPR2_RNGSEL_Msk,
                                   WHAL_STM32U5_RCC_CCIPR2_RNGSEL_Pos,
                                   src));
    return WHAL_SUCCESS;
}

/**
 * @brief Set the enable bit for a peripheral clock gate.
 */
static inline whal_Error whal_Stm32u5_Rcc_EnablePeriphClk(
    const whal_Stm32u5_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 1));
    return WHAL_SUCCESS;
}

/**
 * @brief Clear the enable bit for a peripheral clock gate.
 */
static inline whal_Error whal_Stm32u5_Rcc_DisablePeriphClk(
    const whal_Stm32u5_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32U5_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 0));
    return WHAL_SUCCESS;
}

/**
 * @brief Program the PWR voltage scaling range and wait for VOSRDY.
 *        VOS Range 1 is required before SYSCLK exceeds 24 MHz; the EPOD
 *        booster (BOOSTEN) is required for SYSCLK above 55 MHz.
 */
static inline whal_Error whal_Stm32u5_Pwr_SetVosRange(
    whal_Stm32u5_Pwr_VosRange range)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32U5_PWR_BASE, WHAL_STM32U5_PWR_VOSR_REG,
                      WHAL_STM32U5_PWR_VOSR_VOS_Msk,
                      whal_SetBits(WHAL_STM32U5_PWR_VOSR_VOS_Msk,
                                   WHAL_STM32U5_PWR_VOSR_VOS_Pos, range));
    do {
        whal_Reg32_Get(WHAL_STM32U5_PWR_BASE, WHAL_STM32U5_PWR_VOSR_REG,
                       WHAL_STM32U5_PWR_VOSR_VOSRDY_Msk,
                       WHAL_STM32U5_PWR_VOSR_VOSRDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/**
 * @brief Enable the EPOD booster (PWR_VOSR.BOOSTEN) and wait for BOOSTRDY.
 *        Required for SYSCLK above 55 MHz on Range 1.
 */
static inline whal_Error whal_Stm32u5_Pwr_EnableEpodBooster(void)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32U5_PWR_BASE, WHAL_STM32U5_PWR_VOSR_REG,
                      WHAL_STM32U5_PWR_VOSR_BOOSTEN_Msk,
                      whal_SetBits(WHAL_STM32U5_PWR_VOSR_BOOSTEN_Msk,
                                   WHAL_STM32U5_PWR_VOSR_BOOSTEN_Pos, 1));
    do {
        whal_Reg32_Get(WHAL_STM32U5_PWR_BASE, WHAL_STM32U5_PWR_VOSR_REG,
                       WHAL_STM32U5_PWR_VOSR_BOOSTRDY_Msk,
                       WHAL_STM32U5_PWR_VOSR_BOOSTRDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

#endif /* WHAL_STM32U5_RCC_H */
