/* stm32c0_rcc.h
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

#ifndef WHAL_STM32C0_RCC_H
#define WHAL_STM32C0_RCC_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * @file stm32c0_rcc.h
 * @brief STM32C0 RCC (Reset and Clock Control) driver.
 *
 * Boards bring up the clock tree imperatively from Board_Init by calling
 * the helpers below. The STM32C0 has no PLL — the primary high-speed
 * clock is HSI48 (48 MHz) with a configurable divider (HSIDIV) producing
 * HSISYS. The RCC peripheral lives at a fixed address — clock is a
 * board-level driver with no device struct, no generic API, no vtable.
 */

#define WHAL_STM32C0_RCC_BASE          0x40021000

/* Clock Control Register */
#define WHAL_STM32C0_RCC_CR_REG        0x000
#define WHAL_STM32C0_RCC_CR_HSION_Pos  8
#define WHAL_STM32C0_RCC_CR_HSION_Msk  (1UL << WHAL_STM32C0_RCC_CR_HSION_Pos)
#define WHAL_STM32C0_RCC_CR_HSIRDY_Pos 10
#define WHAL_STM32C0_RCC_CR_HSIRDY_Msk (1UL << WHAL_STM32C0_RCC_CR_HSIRDY_Pos)
#define WHAL_STM32C0_RCC_CR_HSIDIV_Pos 11
#define WHAL_STM32C0_RCC_CR_HSIDIV_Msk (WHAL_BITMASK(3) << WHAL_STM32C0_RCC_CR_HSIDIV_Pos)

/* Clock Configuration Register */
#define WHAL_STM32C0_RCC_CFGR_REG     0x008
#define WHAL_STM32C0_RCC_CFGR_SW_Pos  0
#define WHAL_STM32C0_RCC_CFGR_SW_Msk  (WHAL_BITMASK(3) << WHAL_STM32C0_RCC_CFGR_SW_Pos)
#define WHAL_STM32C0_RCC_CFGR_SWS_Pos 3
#define WHAL_STM32C0_RCC_CFGR_SWS_Msk (WHAL_BITMASK(3) << WHAL_STM32C0_RCC_CFGR_SWS_Pos)

/*
 * @brief HSI divider selection (RCC_CR.HSIDIV).
 */
typedef enum {
    WHAL_STM32C0_RCC_HSIDIV_1,   /* 48 MHz */
    WHAL_STM32C0_RCC_HSIDIV_2,   /* 24 MHz */
    WHAL_STM32C0_RCC_HSIDIV_4,   /* 12 MHz */
    WHAL_STM32C0_RCC_HSIDIV_8,   /* 6 MHz */
    WHAL_STM32C0_RCC_HSIDIV_16,  /* 3 MHz */
    WHAL_STM32C0_RCC_HSIDIV_32,  /* 1.5 MHz */
    WHAL_STM32C0_RCC_HSIDIV_64,  /* 750 kHz */
    WHAL_STM32C0_RCC_HSIDIV_128, /* 375 kHz */
} whal_Stm32c0_Rcc_HsiDiv;

/*
 * @brief System clock source selection (RCC_CFGR.SW).
 */
typedef enum {
    WHAL_STM32C0_RCC_SYSCLK_SRC_HSISYS = 0,
    WHAL_STM32C0_RCC_SYSCLK_SRC_HSE    = 1,
    WHAL_STM32C0_RCC_SYSCLK_SRC_LSI    = 3,
    WHAL_STM32C0_RCC_SYSCLK_SRC_LSE    = 4,
} whal_Stm32c0_Rcc_SysClockSrc;

/*
 * @brief Peripheral clock descriptor (RCC *ENR enable bit).
 */
typedef struct {
    size_t regOffset;
    size_t enableMask;
    size_t enablePos;
} whal_Stm32c0_Rcc_PeriphClk;

/*
 * @brief Enable HSI16/HSI48, wait for it to be ready, then set HSIDIV.
 *
 * Sets RCC_CR.HSION, polls RCC_CR.HSIRDY, then writes HSIDIV. Leaves HSISYS
 * = 48 MHz / 2^hsidiv.
 *
 * @param hsidiv HSI divider selection.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32c0_Rcc_EnableHsi(
    whal_Stm32c0_Rcc_HsiDiv hsidiv)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32C0_RCC_BASE, WHAL_STM32C0_RCC_CR_REG,
                      WHAL_STM32C0_RCC_CR_HSION_Msk,
                      WHAL_STM32C0_RCC_CR_HSION_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32C0_RCC_BASE, WHAL_STM32C0_RCC_CR_REG,
                       WHAL_STM32C0_RCC_CR_HSIRDY_Msk,
                       WHAL_STM32C0_RCC_CR_HSIRDY_Pos, &rdy);
    } while (!rdy);
    whal_Reg32_Update(WHAL_STM32C0_RCC_BASE, WHAL_STM32C0_RCC_CR_REG,
                      WHAL_STM32C0_RCC_CR_HSIDIV_Msk,
                      whal_SetBits(WHAL_STM32C0_RCC_CR_HSIDIV_Msk,
                                   WHAL_STM32C0_RCC_CR_HSIDIV_Pos, hsidiv));
    return WHAL_SUCCESS;
}

/*
 * @brief Switch SYSCLK to the requested source and wait for the switch.
 *
 * Writes RCC_CFGR.SW and polls RCC_CFGR.SWS until it reports the new source.
 *
 * @param src    System clock source.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32c0_Rcc_SetSysClock(
    whal_Stm32c0_Rcc_SysClockSrc src)
{
    uint32_t sws;

    whal_Reg32_Update(WHAL_STM32C0_RCC_BASE, WHAL_STM32C0_RCC_CFGR_REG,
                      WHAL_STM32C0_RCC_CFGR_SW_Msk,
                      whal_SetBits(WHAL_STM32C0_RCC_CFGR_SW_Msk,
                                   WHAL_STM32C0_RCC_CFGR_SW_Pos, src));
    do {
        whal_Reg32_Get(WHAL_STM32C0_RCC_BASE, WHAL_STM32C0_RCC_CFGR_REG,
                       WHAL_STM32C0_RCC_CFGR_SWS_Msk,
                       WHAL_STM32C0_RCC_CFGR_SWS_Pos, &sws);
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
static inline whal_Error whal_Stm32c0_Rcc_EnablePeriphClk(
    const whal_Stm32c0_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32C0_RCC_BASE, clk->regOffset, clk->enableMask,
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
static inline whal_Error whal_Stm32c0_Rcc_DisablePeriphClk(
    const whal_Stm32c0_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32C0_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 0));
    return WHAL_SUCCESS;
}

#endif /* WHAL_STM32C0_RCC_H */
