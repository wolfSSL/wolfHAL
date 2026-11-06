/* stm32n6_rcc.h
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

#ifndef WHAL_STM32N6_RCC_H
#define WHAL_STM32N6_RCC_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/**
 * @file stm32n6_rcc.h
 * @brief STM32N6 RCC (Reset and Clock Control) driver.
 *
 * Boards bring up the clock tree imperatively from Board_Init.
 *
 * Differences from STM32H5/WBA:
 *   - Four PLLs (PLL1-PLL4) with DIVM/DIVN/POSTDIV1/POSTDIV2 scheme
 *   - Separate SYSSW and CPUSW fields in CFGR1
 *   - PLL1 configuration split across PLL1CFGR1/PLL1CFGR2/PLL1CFGR3
 */

#define WHAL_STM32N6_RCC_BASE            0x46028000

#define WHAL_STM32N6_RCC_CR_REG          0x000
#define WHAL_STM32N6_RCC_CR_PLL1ON_Msk   (1UL << 8)

#define WHAL_STM32N6_RCC_SR_REG          0x004
#define WHAL_STM32N6_RCC_SR_PLL1RDY_Msk  (1UL << 8)
#define WHAL_STM32N6_RCC_SR_PLL1RDY_Pos  8

#define WHAL_STM32N6_RCC_CFGR1_REG        0x020
#define WHAL_STM32N6_RCC_CFGR1_CPUSW_Pos  16
#define WHAL_STM32N6_RCC_CFGR1_CPUSW_Msk  (3UL << WHAL_STM32N6_RCC_CFGR1_CPUSW_Pos)
#define WHAL_STM32N6_RCC_CFGR1_CPUSWS_Pos 20
#define WHAL_STM32N6_RCC_CFGR1_CPUSWS_Msk (3UL << WHAL_STM32N6_RCC_CFGR1_CPUSWS_Pos)
#define WHAL_STM32N6_RCC_CFGR1_SYSSW_Pos  24
#define WHAL_STM32N6_RCC_CFGR1_SYSSW_Msk  (3UL << WHAL_STM32N6_RCC_CFGR1_SYSSW_Pos)
#define WHAL_STM32N6_RCC_CFGR1_SYSSWS_Pos 28
#define WHAL_STM32N6_RCC_CFGR1_SYSSWS_Msk (3UL << WHAL_STM32N6_RCC_CFGR1_SYSSWS_Pos)

#define WHAL_STM32N6_RCC_PLL1CFGR1_REG          0x080
#define WHAL_STM32N6_RCC_PLL1CFGR1_PLL1SEL_Pos  28
#define WHAL_STM32N6_RCC_PLL1CFGR1_PLL1SEL_Msk  (7UL << WHAL_STM32N6_RCC_PLL1CFGR1_PLL1SEL_Pos)
#define WHAL_STM32N6_RCC_PLL1CFGR1_PLL1DIVM_Pos 20
#define WHAL_STM32N6_RCC_PLL1CFGR1_PLL1DIVM_Msk (0x3FUL << WHAL_STM32N6_RCC_PLL1CFGR1_PLL1DIVM_Pos)
#define WHAL_STM32N6_RCC_PLL1CFGR1_PLL1DIVN_Pos 0
#define WHAL_STM32N6_RCC_PLL1CFGR1_PLL1DIVN_Msk (0xFFFUL << WHAL_STM32N6_RCC_PLL1CFGR1_PLL1DIVN_Pos)

#define WHAL_STM32N6_RCC_PLL1CFGR3_REG            0x088
#define WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIVEN_Msk (1UL << 30)
#define WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV1_Pos  27
#define WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV1_Msk  (7UL << WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV1_Pos)
#define WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV2_Pos  24
#define WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV2_Msk  (7UL << WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV2_Pos)

#define WHAL_STM32N6_RCC_CCIPR2_REG         0x148
#define WHAL_STM32N6_RCC_CCIPR2_ETH1SEL_Pos 16
#define WHAL_STM32N6_RCC_CCIPR2_ETH1SEL_Msk (7UL << WHAL_STM32N6_RCC_CCIPR2_ETH1SEL_Pos)

/**
 * @brief System clock switch selection (RCC_CFGR1.SYSSW).
 */
typedef enum {
    WHAL_STM32N6_RCC_SYSCLK_SRC_HSI = 0,
    WHAL_STM32N6_RCC_SYSCLK_SRC_MSI = 1,
    WHAL_STM32N6_RCC_SYSCLK_SRC_HSE = 2,
    WHAL_STM32N6_RCC_SYSCLK_SRC_IC2 = 3,
} whal_Stm32n6_Rcc_SysClockSrc;

/**
 * @brief CPU clock switch selection (RCC_CFGR1.CPUSW).
 */
typedef enum {
    WHAL_STM32N6_RCC_CPUCLK_SRC_HSI = 0,
    WHAL_STM32N6_RCC_CPUCLK_SRC_MSI = 1,
    WHAL_STM32N6_RCC_CPUCLK_SRC_HSE = 2,
    WHAL_STM32N6_RCC_CPUCLK_SRC_IC1 = 3,
} whal_Stm32n6_Rcc_CpuClockSrc;

/**
 * @brief Ethernet 1 PHY interface selection (RCC_CCIPR2.ETH1SEL).
 */
typedef enum {
    WHAL_STM32N6_RCC_ETH1_IF_MII   = 0,
    WHAL_STM32N6_RCC_ETH1_IF_RGMII = 1,
    WHAL_STM32N6_RCC_ETH1_IF_RMII  = 4,
} whal_Stm32n6_Rcc_Eth1If;

/**
 * @brief PLL1 input clock source (RCC_PLL1CFGR1.PLL1SEL).
 */
typedef enum {
    WHAL_STM32N6_RCC_PLL1SRC_HSI     = 0,
    WHAL_STM32N6_RCC_PLL1SRC_MSI     = 1,
    WHAL_STM32N6_RCC_PLL1SRC_HSE     = 2,
    WHAL_STM32N6_RCC_PLL1SRC_I2SCKIN = 3,
} whal_Stm32n6_Rcc_Pll1Src;

/**
 * @brief PLL1 configuration parameters.
 *   VCO = (input / m) * n
 *   PLL1 output = VCO / pdiv1 / pdiv2
 */
typedef struct {
    whal_Stm32n6_Rcc_Pll1Src clkSrc;
    uint16_t n;
    uint8_t m;
    uint8_t pdiv1;
    uint8_t pdiv2;
} whal_Stm32n6_Rcc_Pll1Cfg;

/**
 * @brief Peripheral clock descriptor.
 */
typedef struct {
    size_t regOffset;
    size_t enableMask;
    size_t enablePos;
} whal_Stm32n6_Rcc_PeriphClk;

/**
 * @brief Cfg for EnableOsc/DisableOsc — on bit + ready bit.
 */
typedef struct {
    size_t onReg;
    size_t onMsk;
    size_t rdyReg;
    size_t rdyMsk;
    size_t rdyPos;
} whal_Stm32n6_Rcc_OscCfg;

#define WHAL_STM32N6_RCC_LSI_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL <<  0),                  \
    .rdyReg = 0x004, .rdyMsk = (1UL <<  0), .rdyPos =  0
#define WHAL_STM32N6_RCC_LSE_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL <<  1),                  \
    .rdyReg = 0x004, .rdyMsk = (1UL <<  1), .rdyPos =  1
#define WHAL_STM32N6_RCC_MSI_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL <<  2),                  \
    .rdyReg = 0x004, .rdyMsk = (1UL <<  2), .rdyPos =  2
#define WHAL_STM32N6_RCC_HSI_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL <<  3),                  \
    .rdyReg = 0x004, .rdyMsk = (1UL <<  3), .rdyPos =  3
#define WHAL_STM32N6_RCC_HSE_CFG                             \
    .onReg  = 0x000, .onMsk  = (1UL <<  4),                  \
    .rdyReg = 0x004, .rdyMsk = (1UL <<  4), .rdyPos =  4

/**
 * @brief Turn on an oscillator and wait for its ready bit.
 *
 * @param cfg    Oscillator on/ready bit descriptor (e.g. WHAL_STM32N6_RCC_HSE_CFG).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32n6_Rcc_EnableOsc(
    const whal_Stm32n6_Rcc_OscCfg *cfg)
{
    uint32_t rdy;

    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, cfg->onReg, cfg->onMsk, cfg->onMsk);
    do {
        whal_Reg32_Get(WHAL_STM32N6_RCC_BASE, cfg->rdyReg, cfg->rdyMsk,
                       cfg->rdyPos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/**
 * @brief Turn off an oscillator.
 *
 * @param cfg    Oscillator on/ready bit descriptor.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32n6_Rcc_DisableOsc(
    const whal_Stm32n6_Rcc_OscCfg *cfg)
{
    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, cfg->onReg, cfg->onMsk, 0);
    return WHAL_SUCCESS;
}

/**
 * @brief Configure and enable PLL1, waiting for lock.
 *
 * Turns PLL1 off (waits for !PLL1RDY), programs PLL1CFGR1 (SEL/DIVM/DIVN)
 * and PLL1CFGR3 (PDIVEN + PDIV1/PDIV2), sets PLL1ON, polls PLL1RDY.
 *
 * @param cfg    PLL1 configuration.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32n6_Rcc_EnablePll1(
    const whal_Stm32n6_Rcc_Pll1Cfg *cfg)
{
    uint32_t rdy;

    /* Disable PLL1 before reconfiguring; wait until off. */
    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_CR_REG,
                      WHAL_STM32N6_RCC_CR_PLL1ON_Msk, 0);
    do {
        whal_Reg32_Get(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_SR_REG,
                       WHAL_STM32N6_RCC_SR_PLL1RDY_Msk,
                       WHAL_STM32N6_RCC_SR_PLL1RDY_Pos, &rdy);
    } while (rdy);

    whal_Reg32_Write(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_PLL1CFGR1_REG,
                     whal_SetBits(WHAL_STM32N6_RCC_PLL1CFGR1_PLL1SEL_Msk,
                                  WHAL_STM32N6_RCC_PLL1CFGR1_PLL1SEL_Pos, cfg->clkSrc) |
                     whal_SetBits(WHAL_STM32N6_RCC_PLL1CFGR1_PLL1DIVM_Msk,
                                  WHAL_STM32N6_RCC_PLL1CFGR1_PLL1DIVM_Pos, cfg->m) |
                     whal_SetBits(WHAL_STM32N6_RCC_PLL1CFGR1_PLL1DIVN_Msk,
                                  WHAL_STM32N6_RCC_PLL1CFGR1_PLL1DIVN_Pos, cfg->n));
    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_PLL1CFGR3_REG,
                      WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIVEN_Msk |
                      WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV1_Msk |
                      WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV2_Msk,
                      WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIVEN_Msk |
                      whal_SetBits(WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV1_Msk,
                                   WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV1_Pos, cfg->pdiv1) |
                      whal_SetBits(WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV2_Msk,
                                   WHAL_STM32N6_RCC_PLL1CFGR3_PLL1PDIV2_Pos, cfg->pdiv2));

    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_CR_REG,
                      WHAL_STM32N6_RCC_CR_PLL1ON_Msk,
                      WHAL_STM32N6_RCC_CR_PLL1ON_Msk);
    do {
        whal_Reg32_Get(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_SR_REG,
                       WHAL_STM32N6_RCC_SR_PLL1RDY_Msk,
                       WHAL_STM32N6_RCC_SR_PLL1RDY_Pos, &rdy);
    } while (!rdy);
    return WHAL_SUCCESS;
}

/**
 * @brief Turn PLL1 off (clears RCC_CR.PLL1ON).
 *
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32n6_Rcc_DisablePll1(void)
{
    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_CR_REG,
                      WHAL_STM32N6_RCC_CR_PLL1ON_Msk, 0);
    return WHAL_SUCCESS;
}

/**
 * @brief Switch the system clock (SYSSW) to the requested source.
 *
 * Programs CFGR1.SYSSW and polls CFGR1.SYSSWS until it reports the new source.
 *
 * @param src    System clock source.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32n6_Rcc_SetSysClock(
    whal_Stm32n6_Rcc_SysClockSrc src)
{
    uint32_t sws;

    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_CFGR1_REG,
                      WHAL_STM32N6_RCC_CFGR1_SYSSW_Msk,
                      whal_SetBits(WHAL_STM32N6_RCC_CFGR1_SYSSW_Msk,
                                   WHAL_STM32N6_RCC_CFGR1_SYSSW_Pos, src));
    do {
        whal_Reg32_Get(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_CFGR1_REG,
                       WHAL_STM32N6_RCC_CFGR1_SYSSWS_Msk,
                       WHAL_STM32N6_RCC_CFGR1_SYSSWS_Pos, &sws);
    } while (sws != (size_t)src);
    return WHAL_SUCCESS;
}

/**
 * @brief Switch the CPU clock (CPUSW) to the requested source.
 *
 * Programs CFGR1.CPUSW and polls CFGR1.CPUSWS until it reports the new source.
 *
 * @param src    CPU clock source.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32n6_Rcc_SetCpuClock(
    whal_Stm32n6_Rcc_CpuClockSrc src)
{
    uint32_t sws;

    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_CFGR1_REG,
                      WHAL_STM32N6_RCC_CFGR1_CPUSW_Msk,
                      whal_SetBits(WHAL_STM32N6_RCC_CFGR1_CPUSW_Msk,
                                   WHAL_STM32N6_RCC_CFGR1_CPUSW_Pos, src));
    do {
        whal_Reg32_Get(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_CFGR1_REG,
                       WHAL_STM32N6_RCC_CFGR1_CPUSWS_Msk,
                       WHAL_STM32N6_RCC_CFGR1_CPUSWS_Pos, &sws);
    } while (sws != (size_t)src);
    return WHAL_SUCCESS;
}

/**
 * @brief Select the Ethernet 1 PHY interface (MII/RGMII/RMII).
 *
 * @param mode   PHY interface selection (RCC_CCIPR2.ETH1SEL).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32n6_Rcc_SetEth1If(
    whal_Stm32n6_Rcc_Eth1If mode)
{
    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, WHAL_STM32N6_RCC_CCIPR2_REG,
                      WHAL_STM32N6_RCC_CCIPR2_ETH1SEL_Msk,
                      whal_SetBits(WHAL_STM32N6_RCC_CCIPR2_ETH1SEL_Msk,
                                   WHAL_STM32N6_RCC_CCIPR2_ETH1SEL_Pos,
                                   mode));
    return WHAL_SUCCESS;
}

/**
 * @brief Set the enable bit for a peripheral clock gate.
 *
 * @param clk    Peripheral clock descriptor (RCC *ENR register + bit).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32n6_Rcc_EnablePeriphClk(
    const whal_Stm32n6_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 1));
    return WHAL_SUCCESS;
}

/**
 * @brief Clear the enable bit for a peripheral clock gate.
 *
 * @param clk    Peripheral clock descriptor (RCC *ENR register + bit).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32n6_Rcc_DisablePeriphClk(
    const whal_Stm32n6_Rcc_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_STM32N6_RCC_BASE, clk->regOffset, clk->enableMask,
                      whal_SetBits(clk->enableMask, clk->enablePos, 0));
    return WHAL_SUCCESS;
}

#endif /* WHAL_STM32N6_RCC_H */
