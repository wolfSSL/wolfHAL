/* stm32f4_rcc.h
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

#ifndef WHAL_STM32F4_RCC_H
#define WHAL_STM32F4_RCC_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * @file stm32f4_rcc.h
 * @brief STM32F4 RCC (Reset and Clock Control) driver.
 *
 * Boards bring up the clock tree imperatively from Board_Init. The RCC
 * peripheral lives at a fixed address — clock is a board-level driver
 * with no device struct, no generic API, no vtable.
 *
 * PLL output:
 *   f_vco  = (f_input / m) * n
 *   f_pll  = f_vco / p   (SYSCLK)
 *   f_pllq = f_vco / q   (USB / SDIO)
 */

#define WHAL_STM32F4_RCC_BASE            0x40023800

#define WHAL_STM32F4_RCC_CR_REG          0x000
#define WHAL_STM32F4_RCC_CR_PLLON_Msk    (1UL << 24)
#define WHAL_STM32F4_RCC_CR_PLLRDY_Msk   (1UL << 25)
#define WHAL_STM32F4_RCC_CR_PLLRDY_Pos   25

#define WHAL_STM32F4_RCC_PLLCFGR_REG        0x004
#define WHAL_STM32F4_RCC_PLLCFGR_PLLM_Pos   0
#define WHAL_STM32F4_RCC_PLLCFGR_PLLM_Msk   (WHAL_BITMASK(6) << WHAL_STM32F4_RCC_PLLCFGR_PLLM_Pos)
#define WHAL_STM32F4_RCC_PLLCFGR_PLLN_Pos   6
#define WHAL_STM32F4_RCC_PLLCFGR_PLLN_Msk   (WHAL_BITMASK(9) << WHAL_STM32F4_RCC_PLLCFGR_PLLN_Pos)
#define WHAL_STM32F4_RCC_PLLCFGR_PLLP_Pos   16
#define WHAL_STM32F4_RCC_PLLCFGR_PLLP_Msk   (WHAL_BITMASK(2) << WHAL_STM32F4_RCC_PLLCFGR_PLLP_Pos)
#define WHAL_STM32F4_RCC_PLLCFGR_PLLSRC_Pos 22
#define WHAL_STM32F4_RCC_PLLCFGR_PLLSRC_Msk (1UL << WHAL_STM32F4_RCC_PLLCFGR_PLLSRC_Pos)
#define WHAL_STM32F4_RCC_PLLCFGR_PLLQ_Pos   24
#define WHAL_STM32F4_RCC_PLLCFGR_PLLQ_Msk   (WHAL_BITMASK(4) << WHAL_STM32F4_RCC_PLLCFGR_PLLQ_Pos)
#define WHAL_STM32F4_RCC_PLLCFGR_Msk \
    (WHAL_STM32F4_RCC_PLLCFGR_PLLM_Msk | WHAL_STM32F4_RCC_PLLCFGR_PLLN_Msk |   \
     WHAL_STM32F4_RCC_PLLCFGR_PLLP_Msk | WHAL_STM32F4_RCC_PLLCFGR_PLLSRC_Msk | \
     WHAL_STM32F4_RCC_PLLCFGR_PLLQ_Msk)

#define WHAL_STM32F4_RCC_CFGR_REG       0x008
#define WHAL_STM32F4_RCC_CFGR_SW_Pos    0
#define WHAL_STM32F4_RCC_CFGR_SW_Msk    (WHAL_BITMASK(2) << WHAL_STM32F4_RCC_CFGR_SW_Pos)
#define WHAL_STM32F4_RCC_CFGR_SWS_Pos   2
#define WHAL_STM32F4_RCC_CFGR_SWS_Msk   (WHAL_BITMASK(2) << WHAL_STM32F4_RCC_CFGR_SWS_Pos)
#define WHAL_STM32F4_RCC_CFGR_PPRE1_Pos 10
#define WHAL_STM32F4_RCC_CFGR_PPRE1_Msk (WHAL_BITMASK(3) << WHAL_STM32F4_RCC_CFGR_PPRE1_Pos)
#define WHAL_STM32F4_RCC_CFGR_PPRE2_Pos 13
#define WHAL_STM32F4_RCC_CFGR_PPRE2_Msk (WHAL_BITMASK(3) << WHAL_STM32F4_RCC_CFGR_PPRE2_Pos)

/*
 * @brief System clock source selection (RCC_CFGR.SW).
 */
typedef enum {
    WHAL_STM32F4_RCC_SYSCLK_SRC_HSI,
    WHAL_STM32F4_RCC_SYSCLK_SRC_HSE,
    WHAL_STM32F4_RCC_SYSCLK_SRC_PLL,
} whal_Stm32f4_Rcc_SysClockSrc;

/*
 * @brief PLL input selection (RCC_PLLCFGR.PLLSRC).
 */
typedef enum {
    WHAL_STM32F4_RCC_PLLCLK_SRC_HSI,
    WHAL_STM32F4_RCC_PLLCLK_SRC_HSE,
} whal_Stm32f4_Rcc_PllClockSrc;

/*
 * @brief PLL configuration parameters.
 *   m: 2-63   (VCO input 1-2 MHz)
 *   n: 50-432 (VCO output 100-432 MHz)
 *   p: 0-3    (div by 2/4/6/8)
 *   q: 2-15
 */
typedef struct {
    whal_Stm32f4_Rcc_PllClockSrc clkSrc;
    uint16_t n;
    uint8_t m;
    uint8_t p;
    uint8_t q;
} whal_Stm32f4_Rcc_PllCfg;

/*
 * @brief Bus prescaler config (APB1/APB2).
 *   ppre1/ppre2: 0=/1, 4=/2, 5=/4, 6=/8, 7=/16
 */
typedef struct {
    uint8_t ppre1;
    uint8_t ppre2;
} whal_Stm32f4_Rcc_BusCfg;

/*
 * @brief Peripheral clock descriptor.
 */
typedef struct {
    size_t regOffset;
    size_t enableMask;
    size_t enablePos;
} whal_Stm32f4_Rcc_PeriphClk;

/*
 * @brief Cfg for EnableOsc/DisableOsc — on bit + ready bit.
 */
typedef struct {
    size_t onReg;
    size_t onMsk;
    size_t rdyReg;
    size_t rdyMsk;
    size_t rdyPos;
} whal_Stm32f4_Rcc_OscCfg;

#define WHAL_STM32F4_RCC_HSI_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL <<  0),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL <<  1), .rdyPos =  1
#define WHAL_STM32F4_RCC_HSE_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL << 16),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL << 17), .rdyPos = 17

/*
 * @brief Turn on an oscillator (HSI/HSE) and wait for its ready bit.
 *
 * @param cfg    Oscillator on/ready bit descriptor.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32f4_Rcc_EnableOsc(
    const whal_Stm32f4_Rcc_OscCfg *cfg)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32F4_RCC_BASE, cfg->onReg, cfg->onMsk, cfg->onMsk);
    do {
        whal_Reg32_Get(WHAL_STM32F4_RCC_BASE, cfg->rdyReg, cfg->rdyMsk,
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
static inline whal_Error whal_Stm32f4_Rcc_DisableOsc(
    const whal_Stm32f4_Rcc_OscCfg *cfg)
{
    whal_Reg32_Update(WHAL_STM32F4_RCC_BASE, cfg->onReg, cfg->onMsk, 0);
    return WHAL_SUCCESS;
}

/*
 * @brief Configure and enable the PLL, waiting for lock.
 *
 * Turns the PLL off, programs PLLCFGR (M/N/P/Q + source), sets PLLON, and
 * polls PLLRDY. Caller must have enabled the reference clock first.
 *
 * @param cfg    PLL configuration.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32f4_Rcc_EnablePll(
    const whal_Stm32f4_Rcc_PllCfg *cfg)
{
    uint32_t rdy;

    /* Disable PLL before reconfiguring. */
    whal_Reg32_Update(WHAL_STM32F4_RCC_BASE, WHAL_STM32F4_RCC_CR_REG,
                      WHAL_STM32F4_RCC_CR_PLLON_Msk, 0);

    whal_Reg32_Update(WHAL_STM32F4_RCC_BASE, WHAL_STM32F4_RCC_PLLCFGR_REG,
                      WHAL_STM32F4_RCC_PLLCFGR_Msk,
                      whal_SetBits(WHAL_STM32F4_RCC_PLLCFGR_PLLM_Msk,
                                   WHAL_STM32F4_RCC_PLLCFGR_PLLM_Pos, cfg->m) |
                      whal_SetBits(WHAL_STM32F4_RCC_PLLCFGR_PLLN_Msk,
                                   WHAL_STM32F4_RCC_PLLCFGR_PLLN_Pos, cfg->n) |
                      whal_SetBits(WHAL_STM32F4_RCC_PLLCFGR_PLLP_Msk,
                                   WHAL_STM32F4_RCC_PLLCFGR_PLLP_Pos, cfg->p) |
                      whal_SetBits(WHAL_STM32F4_RCC_PLLCFGR_PLLSRC_Msk,
                                   WHAL_STM32F4_RCC_PLLCFGR_PLLSRC_Pos, cfg->clkSrc) |
                      whal_SetBits(WHAL_STM32F4_RCC_PLLCFGR_PLLQ_Msk,
                                   WHAL_STM32F4_RCC_PLLCFGR_PLLQ_Pos, cfg->q));

    whal_Reg32_Update(WHAL_STM32F4_RCC_BASE, WHAL_STM32F4_RCC_CR_REG,
                      WHAL_STM32F4_RCC_CR_PLLON_Msk,
                      WHAL_STM32F4_RCC_CR_PLLON_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32F4_RCC_BASE, WHAL_STM32F4_RCC_CR_REG,
                       WHAL_STM32F4_RCC_CR_PLLRDY_Msk,
                       WHAL_STM32F4_RCC_CR_PLLRDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/*
 * @brief Turn the PLL off (clears RCC_CR.PLLON).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32f4_Rcc_DisablePll(void)
{
    whal_Reg32_Update(WHAL_STM32F4_RCC_BASE, WHAL_STM32F4_RCC_CR_REG,
                      WHAL_STM32F4_RCC_CR_PLLON_Msk, 0);
    return WHAL_SUCCESS;
}

/*
 * @brief Program APB1/APB2 bus prescalers (RCC_CFGR.PPRE1/PPRE2).
 *
 * @param cfg    Bus prescaler configuration.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32f4_Rcc_SetBusPrescalers(
    const whal_Stm32f4_Rcc_BusCfg *cfg)
{
    whal_Reg32_Update(WHAL_STM32F4_RCC_BASE, WHAL_STM32F4_RCC_CFGR_REG,
                      WHAL_STM32F4_RCC_CFGR_PPRE1_Msk |
                      WHAL_STM32F4_RCC_CFGR_PPRE2_Msk,
                      whal_SetBits(WHAL_STM32F4_RCC_CFGR_PPRE1_Msk,
                                   WHAL_STM32F4_RCC_CFGR_PPRE1_Pos, cfg->ppre1) |
                      whal_SetBits(WHAL_STM32F4_RCC_CFGR_PPRE2_Msk,
                                   WHAL_STM32F4_RCC_CFGR_PPRE2_Pos, cfg->ppre2));
    return WHAL_SUCCESS;
}

/*
 * @brief Switch SYSCLK to the requested source and wait for the switch.
 *
 * @param src    System clock source.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32f4_Rcc_SetSysClock(
    whal_Stm32f4_Rcc_SysClockSrc src)
{
    uint32_t sws;

    whal_Reg32_Update(WHAL_STM32F4_RCC_BASE, WHAL_STM32F4_RCC_CFGR_REG,
                      WHAL_STM32F4_RCC_CFGR_SW_Msk,
                      whal_SetBits(WHAL_STM32F4_RCC_CFGR_SW_Msk,
                                   WHAL_STM32F4_RCC_CFGR_SW_Pos, src));
    do {
        whal_Reg32_Get(WHAL_STM32F4_RCC_BASE, WHAL_STM32F4_RCC_CFGR_REG,
                       WHAL_STM32F4_RCC_CFGR_SWS_Msk,
                       WHAL_STM32F4_RCC_CFGR_SWS_Pos, &sws);
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
static inline whal_Error whal_Stm32f4_Rcc_EnablePeriphClk(
    const whal_Stm32f4_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32F4_RCC_BASE, clk->regOffset, clk->enableMask,
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
static inline whal_Error whal_Stm32f4_Rcc_DisablePeriphClk(
    const whal_Stm32f4_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32F4_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 0));
    return WHAL_SUCCESS;
}

#endif /* WHAL_STM32F4_RCC_H */
