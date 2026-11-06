/* stm32wb0_rcc.h
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

#ifndef WHAL_STM32WB0_RCC_H
#define WHAL_STM32WB0_RCC_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/**
 * @file stm32wb0_rcc.h
 * @brief STM32WB0 RCC (Reset and Clock Control) driver.
 *
 * Boards bring up the clock tree imperatively from Board_Init by calling
 * the helpers below. The STM32WB0 has a Bluetooth-LE-oriented RCC that
 * differs significantly from the wider STM32 line:
 *   - Two high-speed sources: HSI64M (internal 64 MHz RC, optionally
 *     PLL-locked onto HSE — represented by HSIPLLON+HSIPLLRDY) and a
 *     direct HSE path (32 MHz crystal).
 *   - System clock frequency is selected through CLKSYSDIV[2:0] in
 *     RCC_CFGR rather than a PLL output divider. Frequencies of 64 / 32
 *     / 16 / 8 / 4 / 2 / 1 MHz are derived from the fast clock tree.
 *   - HSESEL chooses between the RC64MPLL tree and the direct HSE tree.
 *   - Peripheral clock gates live in AHBENR (0x50), APB0ENR (0x54),
 *     APB1ENR (0x58), and APB2ENR (0x60).
 *
 * The RCC peripheral lives at a fixed address (0x48400000) — clock is a
 * board-level driver with no device struct, no generic API, no vtable.
 */

#define WHAL_STM32WB0_RCC_BASE              0x48400000

/* Clock source control register */
#define WHAL_STM32WB0_RCC_CR_REG            0x000
#define WHAL_STM32WB0_RCC_CR_LSION_Pos      2
#define WHAL_STM32WB0_RCC_CR_LSION_Msk      (1UL << WHAL_STM32WB0_RCC_CR_LSION_Pos)
#define WHAL_STM32WB0_RCC_CR_LSIRDY_Pos     3
#define WHAL_STM32WB0_RCC_CR_LSIRDY_Msk     (1UL << WHAL_STM32WB0_RCC_CR_LSIRDY_Pos)
#define WHAL_STM32WB0_RCC_CR_LSEON_Pos      4
#define WHAL_STM32WB0_RCC_CR_LSEON_Msk      (1UL << WHAL_STM32WB0_RCC_CR_LSEON_Pos)
#define WHAL_STM32WB0_RCC_CR_LSERDY_Pos     5
#define WHAL_STM32WB0_RCC_CR_LSERDY_Msk     (1UL << WHAL_STM32WB0_RCC_CR_LSERDY_Pos)
#define WHAL_STM32WB0_RCC_CR_HSIRDY_Pos     10
#define WHAL_STM32WB0_RCC_CR_HSIRDY_Msk     (1UL << WHAL_STM32WB0_RCC_CR_HSIRDY_Pos)
#define WHAL_STM32WB0_RCC_CR_HSEPLLBUFON_Pos 12
#define WHAL_STM32WB0_RCC_CR_HSEPLLBUFON_Msk (1UL << WHAL_STM32WB0_RCC_CR_HSEPLLBUFON_Pos)
#define WHAL_STM32WB0_RCC_CR_HSIPLLON_Pos   13
#define WHAL_STM32WB0_RCC_CR_HSIPLLON_Msk   (1UL << WHAL_STM32WB0_RCC_CR_HSIPLLON_Pos)
#define WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Pos  14
#define WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Msk  (1UL << WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Pos)
#define WHAL_STM32WB0_RCC_CR_HSEON_Pos      16
#define WHAL_STM32WB0_RCC_CR_HSEON_Msk      (1UL << WHAL_STM32WB0_RCC_CR_HSEON_Pos)
#define WHAL_STM32WB0_RCC_CR_HSERDY_Pos     17
#define WHAL_STM32WB0_RCC_CR_HSERDY_Msk     (1UL << WHAL_STM32WB0_RCC_CR_HSERDY_Pos)

/* Clock configuration register */
#define WHAL_STM32WB0_RCC_CFGR_REG          0x008
#define WHAL_STM32WB0_RCC_CFGR_HSESEL_Pos   1
#define WHAL_STM32WB0_RCC_CFGR_HSESEL_Msk   (1UL << WHAL_STM32WB0_RCC_CFGR_HSESEL_Pos)
#define WHAL_STM32WB0_RCC_CFGR_STOPHSI_Pos  2
#define WHAL_STM32WB0_RCC_CFGR_STOPHSI_Msk  (1UL << WHAL_STM32WB0_RCC_CFGR_STOPHSI_Pos)
#define WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Pos 3
#define WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Msk (1UL << WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Pos)
#define WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Pos 5
#define WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Msk (WHAL_BITMASK(3) << WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Pos)
#define WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Pos 8
#define WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Msk (WHAL_BITMASK(3) << WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Pos)

/* AHB0 macro cells clock enable register */
#define WHAL_STM32WB0_RCC_AHBENR_REG        0x050
/* APB0 macro cell clock enable register */
#define WHAL_STM32WB0_RCC_APB0ENR_REG       0x054
/* APB1 macro cells clock enable register */
#define WHAL_STM32WB0_RCC_APB1ENR_REG       0x058
/* APB2 macro cells clock enable register */
#define WHAL_STM32WB0_RCC_APB2ENR_REG       0x060

/**
 * @brief System clock frequency selection (RCC_CFGR.CLKSYSDIV).
 *
 * The fast clock tree (HSI64M or HSE) is divided by 2^N to produce the
 * system clock. 64 MHz is only available when HSESEL=0 (HSI64M tree).
 */
typedef enum {
    WHAL_STM32WB0_RCC_SYSCLK_64MHZ = 0,
    WHAL_STM32WB0_RCC_SYSCLK_32MHZ = 1,
    WHAL_STM32WB0_RCC_SYSCLK_16MHZ = 2,
    WHAL_STM32WB0_RCC_SYSCLK_8MHZ  = 3,
    WHAL_STM32WB0_RCC_SYSCLK_4MHZ  = 4,
    WHAL_STM32WB0_RCC_SYSCLK_2MHZ  = 5,
    WHAL_STM32WB0_RCC_SYSCLK_1MHZ  = 6,
} whal_Stm32wb0_Rcc_SysClkDiv;

/**
 * @brief Fast clock source selection (RCC_CFGR.HSESEL).
 */
typedef enum {
    WHAL_STM32WB0_RCC_FASTCLK_RC64MPLL = 0, /* HSI64M (RC) or PLL-locked onto HSE */
    WHAL_STM32WB0_RCC_FASTCLK_HSE      = 1, /* Direct HSE (max 32 MHz sysclk) */
} whal_Stm32wb0_Rcc_FastClkSrc;

/**
 * @brief Peripheral clock descriptor (RCC *ENR enable bit).
 */
typedef struct {
    size_t regOffset;
    size_t enableMask;
    size_t enablePos;
} whal_Stm32wb0_Rcc_PeriphClk;

/**
 * @brief Enable the internal HSI64M oscillator (always on at reset on
 * WB0; this just polls the ready bit defensively).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_EnableHsi(void)
{
    uint32_t rdy;

    do {
        whal_Reg32_Get(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                       WHAL_STM32WB0_RCC_CR_HSIRDY_Msk,
                       WHAL_STM32WB0_RCC_CR_HSIRDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/**
 * @brief Enable the LSI oscillator and wait for it to be ready.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_EnableLsi(void)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                      WHAL_STM32WB0_RCC_CR_LSION_Msk,
                      WHAL_STM32WB0_RCC_CR_LSION_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                       WHAL_STM32WB0_RCC_CR_LSIRDY_Msk,
                       WHAL_STM32WB0_RCC_CR_LSIRDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/**
 * @brief Disable the LSI oscillator.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_DisableLsi(void)
{
    whal_Reg32_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                      WHAL_STM32WB0_RCC_CR_LSION_Msk, 0);
    return WHAL_SUCCESS;
}

/**
 * @brief Enable the external HSE oscillator (32 MHz XO) and wait for it
 * to be ready.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_EnableHse(void)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                      WHAL_STM32WB0_RCC_CR_HSEON_Msk,
                      WHAL_STM32WB0_RCC_CR_HSEON_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                       WHAL_STM32WB0_RCC_CR_HSERDY_Msk,
                       WHAL_STM32WB0_RCC_CR_HSERDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/**
 * @brief Enable the RC64MPLL block that locks the internal 64 MHz RC onto
 * the HSE reference. HSE must be enabled first.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_EnableHsiPll(void)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                      WHAL_STM32WB0_RCC_CR_HSIPLLON_Msk,
                      WHAL_STM32WB0_RCC_CR_HSIPLLON_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                       WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Msk,
                       WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/**
 * @brief Select the fast clock tree source (RC64MPLL vs direct HSE).
 *
 * Writes RCC_CFGR.HSESEL and polls HSESEL_STATUS until the switch
 * completes. Use WHAL_STM32WB0_RCC_FASTCLK_HSE when running directly off
 * the 32 MHz crystal without the PLL; WHAL_STM32WB0_RCC_FASTCLK_RC64MPLL
 * for the internal 64 MHz tree (with or without PLL lock).
 *
 * @param src Fast clock source selection.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_SetFastClkSrc(
    whal_Stm32wb0_Rcc_FastClkSrc src)
{
    uint32_t status;

    whal_Reg32_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                      WHAL_STM32WB0_RCC_CFGR_HSESEL_Msk,
                      whal_SetBits(WHAL_STM32WB0_RCC_CFGR_HSESEL_Msk,
                                   WHAL_STM32WB0_RCC_CFGR_HSESEL_Pos, src));
    do {
        whal_Reg32_Get(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                       WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Msk,
                       WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Pos, &status);
    } while (status != (size_t)src);
    return WHAL_SUCCESS;
}

/**
 * @brief Set the system clock divider (CLKSYSDIV) and wait for the
 * frequency switch to apply.
 *
 * The hardware reports the active frequency in CLKSYSDIV_STATUS and may
 * take up to ~128 system clock cycles to settle.
 *
 * @param div System clock divider selection.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_SetSysClock(
    whal_Stm32wb0_Rcc_SysClkDiv div)
{
    uint32_t status;

    whal_Reg32_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                      WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Msk,
                      whal_SetBits(WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Msk,
                                   WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Pos, div));
    do {
        whal_Reg32_Get(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                       WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Msk,
                       WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Pos, &status);
    } while (status != (size_t)div);
    return WHAL_SUCCESS;
}

/**
 * @brief Set the enable bit for a peripheral clock gate.
 *
 * @param clk Peripheral clock descriptor (RCC *ENR register + bit).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_EnablePeriphClk(
    const whal_Stm32wb0_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32WB0_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 1));
    return WHAL_SUCCESS;
}

/**
 * @brief Clear the enable bit for a peripheral clock gate.
 *
 * @param clk Peripheral clock descriptor (RCC *ENR register + bit).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_DisablePeriphClk(
    const whal_Stm32wb0_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32WB0_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 0));
    return WHAL_SUCCESS;
}

#endif /* WHAL_STM32WB0_RCC_H */
