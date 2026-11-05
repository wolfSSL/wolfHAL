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
#include <wolfHAL/timeout.h>

/**
 * @file stm32wb0_rcc.h
 * @brief STM32WB0 RCC (Reset and Clock Control) driver.
 *
 * Boards bring up the clock tree imperatively from Board_Init by calling
 * the helpers below in order. The RCC peripheral lives at a fixed address;
 * clock is a board-level driver with no device struct, no generic API and
 * no vtable.
 *
 * The fast clock tree has two sources selected by RCC_CFGR.HSESEL:
 *   - RC64MPLL: the 64 MHz block that runs either as a free-running RC
 *     (HSI) or as a PLL locked onto the 32 MHz HSE crystal (PLL64M).
 *   - HSE: the 32 MHz crystal directly, capping SYSCLK at 32 MHz.
 * SYSCLK is the fast clock divided by RCC_CFGR.CLKSYSDIV (64 MHz down to
 * 1 MHz). USART, I2C, LPUART, TRNG and the flash controller also run from a
 * fixed 16 MHz kernel clock that does not follow CLKSYSDIV.
 *
 * The slow clock tree (RTC, IWDG, LPUART in Deepstop, radio timer) is fed
 * by RCC_CFGR.CLKSLOWSEL from LSE, LSI, or the 16 MHz clock divided by 512.
 */

#define WHAL_STM32WB0_RCC_BASE 0x48400000

#define WHAL_STM32WB0_RCC_CR_REG                0x000
#define WHAL_STM32WB0_RCC_CR_LSION_Pos          2
#define WHAL_STM32WB0_RCC_CR_LSION_Msk          (1UL << WHAL_STM32WB0_RCC_CR_LSION_Pos)
#define WHAL_STM32WB0_RCC_CR_LSIRDY_Pos         3
#define WHAL_STM32WB0_RCC_CR_LSIRDY_Msk         (1UL << WHAL_STM32WB0_RCC_CR_LSIRDY_Pos)
#define WHAL_STM32WB0_RCC_CR_LSEON_Pos          4
#define WHAL_STM32WB0_RCC_CR_LSEON_Msk          (1UL << WHAL_STM32WB0_RCC_CR_LSEON_Pos)
#define WHAL_STM32WB0_RCC_CR_LSERDY_Pos         5
#define WHAL_STM32WB0_RCC_CR_LSERDY_Msk         (1UL << WHAL_STM32WB0_RCC_CR_LSERDY_Pos)
#define WHAL_STM32WB0_RCC_CR_LSEBYP_Pos         6
#define WHAL_STM32WB0_RCC_CR_LSEBYP_Msk         (1UL << WHAL_STM32WB0_RCC_CR_LSEBYP_Pos)
#define WHAL_STM32WB0_RCC_CR_LOCKDET_NSTOP_Pos  7
#define WHAL_STM32WB0_RCC_CR_LOCKDET_NSTOP_Msk  (WHAL_BITMASK(3) << WHAL_STM32WB0_RCC_CR_LOCKDET_NSTOP_Pos)
#define WHAL_STM32WB0_RCC_CR_HSIRDY_Pos         10
#define WHAL_STM32WB0_RCC_CR_HSIRDY_Msk         (1UL << WHAL_STM32WB0_RCC_CR_HSIRDY_Pos)
#define WHAL_STM32WB0_RCC_CR_HSEPLLBUFON_Pos    12
#define WHAL_STM32WB0_RCC_CR_HSEPLLBUFON_Msk    (1UL << WHAL_STM32WB0_RCC_CR_HSEPLLBUFON_Pos)
#define WHAL_STM32WB0_RCC_CR_HSIPLLON_Pos       13
#define WHAL_STM32WB0_RCC_CR_HSIPLLON_Msk       (1UL << WHAL_STM32WB0_RCC_CR_HSIPLLON_Pos)
#define WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Pos      14
#define WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Msk      (1UL << WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Pos)
#define WHAL_STM32WB0_RCC_CR_HSEON_Pos          16
#define WHAL_STM32WB0_RCC_CR_HSEON_Msk          (1UL << WHAL_STM32WB0_RCC_CR_HSEON_Pos)
#define WHAL_STM32WB0_RCC_CR_HSERDY_Pos         17
#define WHAL_STM32WB0_RCC_CR_HSERDY_Msk         (1UL << WHAL_STM32WB0_RCC_CR_HSERDY_Pos)

#define WHAL_STM32WB0_RCC_CFGR_REG                   0x008
#define WHAL_STM32WB0_RCC_CFGR_HSESEL_Pos            1
#define WHAL_STM32WB0_RCC_CFGR_HSESEL_Msk            (1UL << WHAL_STM32WB0_RCC_CFGR_HSESEL_Pos)
#define WHAL_STM32WB0_RCC_CFGR_STOPHSI_Pos           2
#define WHAL_STM32WB0_RCC_CFGR_STOPHSI_Msk           (1UL << WHAL_STM32WB0_RCC_CFGR_STOPHSI_Pos)
#define WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Pos     3
#define WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Msk     (1UL << WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Pos)
#define WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Pos         5
#define WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Msk         (WHAL_BITMASK(3) << WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Pos)
#define WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Pos  8
#define WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Msk  (WHAL_BITMASK(3) << WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Pos)
#define WHAL_STM32WB0_RCC_CFGR_LPUCLKSEL_Pos         13
#define WHAL_STM32WB0_RCC_CFGR_LPUCLKSEL_Msk         (1UL << WHAL_STM32WB0_RCC_CFGR_LPUCLKSEL_Pos)
#define WHAL_STM32WB0_RCC_CFGR_CLKSLOWSEL_Pos        15
#define WHAL_STM32WB0_RCC_CFGR_CLKSLOWSEL_Msk        (WHAL_BITMASK(2) << WHAL_STM32WB0_RCC_CFGR_CLKSLOWSEL_Pos)

/**
 * @brief Fast clock tree source (RCC_CFGR.HSESEL).
 */
typedef enum {
    WHAL_STM32WB0_RCC_SYSCLK_SRC_RC64MPLL = 0, /* HSI, or PLL64M once locked */
    WHAL_STM32WB0_RCC_SYSCLK_SRC_HSE      = 1, /* 32 MHz crystal, max 32 MHz */
} whal_Stm32wb0_Rcc_SysClockSrc;

/**
 * @brief System clock frequency selection (RCC_CFGR.CLKSYSDIV).
 *
 * 64 MHz is only reachable from the RC64MPLL source.
 */
typedef enum {
    WHAL_STM32WB0_RCC_SYSCLK_64MHZ = 0,
    WHAL_STM32WB0_RCC_SYSCLK_32MHZ = 1,
    WHAL_STM32WB0_RCC_SYSCLK_16MHZ = 2,
    WHAL_STM32WB0_RCC_SYSCLK_8MHZ  = 3,
    WHAL_STM32WB0_RCC_SYSCLK_4MHZ  = 4,
    WHAL_STM32WB0_RCC_SYSCLK_2MHZ  = 5,
    WHAL_STM32WB0_RCC_SYSCLK_1MHZ  = 6,
} whal_Stm32wb0_Rcc_SysClockDiv;

/**
 * @brief Slow clock tree source (RCC_CFGR.CLKSLOWSEL).
 */
typedef enum {
    WHAL_STM32WB0_RCC_SLOWCLK_SRC_NONE         = 0,
    WHAL_STM32WB0_RCC_SLOWCLK_SRC_LSE          = 1,
    WHAL_STM32WB0_RCC_SLOWCLK_SRC_LSI          = 2,
    WHAL_STM32WB0_RCC_SLOWCLK_SRC_16MHZ_DIV512 = 3,
} whal_Stm32wb0_Rcc_SlowClockSrc;

/**
 * @brief LPUART kernel clock source (RCC_CFGR.LPUCLKSEL).
 */
typedef enum {
    WHAL_STM32WB0_RCC_LPUARTCLK_SRC_16MHZ = 0,
    WHAL_STM32WB0_RCC_LPUARTCLK_SRC_LSE   = 1,
} whal_Stm32wb0_Rcc_LpuartClockSrc;

/**
 * @brief Peripheral clock descriptor (RCC *ENR enable bit).
 */
typedef struct {
    size_t regOffset;
    size_t enableMask;
    size_t enablePos;
} whal_Stm32wb0_Rcc_PeriphClk;

/**
 * @brief Cfg for EnableOsc/DisableOsc: on bit + ready bit.
 */
typedef struct {
    size_t onReg;
    size_t onMsk;
    size_t rdyReg;
    size_t rdyMsk;
    size_t rdyPos;
} whal_Stm32wb0_Rcc_OscCfg;

#define WHAL_STM32WB0_RCC_HSE_CFG                            \
    .onReg  = 0x000, .onMsk  = (1UL << 16),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL << 17), .rdyPos = 17
#define WHAL_STM32WB0_RCC_LSI_CFG                            \
    .onReg  = 0x000, .onMsk  = (1UL <<  2),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL <<  3), .rdyPos =  3
#define WHAL_STM32WB0_RCC_LSE_CFG                            \
    .onReg  = 0x000, .onMsk  = (1UL <<  4),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL <<  5), .rdyPos =  5
/* External 32 kHz clock injected on OSC32_IN; mutually exclusive with LSE. */
#define WHAL_STM32WB0_RCC_LSE_BYPASS_CFG                     \
    .onReg  = 0x000, .onMsk  = (1UL <<  6),                  \
    .rdyReg = 0x000, .rdyMsk = (1UL <<  5), .rdyPos =  5

/**
 * @brief RC64MPLL lock configuration.
 *
 * The PLL has a fixed 32 MHz HSE reference and a fixed 64 MHz output; the
 * only tunable is the lock detector window, (lockDetNstop + 1) * 64 cycles,
 * that the analog lock signal must stay stable for before HSIPLLRDY sets.
 */
typedef struct {
    uint8_t lockDetNstop; /* 0..7 */
} whal_Stm32wb0_Rcc_PllCfg;

/**
 * @brief Turn on an oscillator and wait for its ready bit.
 *
 * @param cfg     Oscillator on/ready bit descriptor (e.g. WHAL_STM32WB0_RCC_HSE_CFG).
 * @param timeout Timeout instance (NULL for unbounded wait).
 *
 * @retval WHAL_SUCCESS  Oscillator is ready.
 * @retval WHAL_ETIMEOUT Ready bit did not set within the timeout.
 */
static inline whal_Error whal_Stm32wb0_Rcc_EnableOsc(
    const whal_Stm32wb0_Rcc_OscCfg *cfg, whal_Timeout *timeout)
{
    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, cfg->onReg, cfg->onMsk, cfg->onMsk);
    return whal_Reg_ReadPoll(WHAL_STM32WB0_RCC_BASE, cfg->rdyReg, cfg->rdyMsk,
                             cfg->rdyMsk, timeout);
}

/**
 * @brief Turn off an oscillator.
 *
 * @param cfg Oscillator on/ready bit descriptor.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_DisableOsc(
    const whal_Stm32wb0_Rcc_OscCfg *cfg)
{
    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, cfg->onReg, cfg->onMsk, 0);
    return WHAL_SUCCESS;
}

/**
 * @brief Release the RC64MPLL stop request and wait for the 64 MHz RC.
 *
 * The RC64MPLL block runs out of reset; this clears RCC_CFGR.STOPHSI and
 * polls RCC_CR.HSIRDY.
 *
 * @param timeout Timeout instance (NULL for unbounded wait).
 *
 * @retval WHAL_SUCCESS  HSI is ready.
 * @retval WHAL_ETIMEOUT HSIRDY did not set within the timeout.
 */
static inline whal_Error whal_Stm32wb0_Rcc_EnableHsi(whal_Timeout *timeout)
{
    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                    WHAL_STM32WB0_RCC_CFGR_STOPHSI_Msk, 0);
    return whal_Reg_ReadPoll(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                             WHAL_STM32WB0_RCC_CR_HSIRDY_Msk,
                             WHAL_STM32WB0_RCC_CR_HSIRDY_Msk, timeout);
}

/**
 * @brief Request the RC64MPLL block to stop (RCC_CFGR.STOPHSI).
 *
 * Only takes effect while the fast clock tree runs from HSE. Hardware
 * restarts the block if SYSCLK is set to 64 MHz or HSE is lost.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_DisableHsi(void)
{
    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                    WHAL_STM32WB0_RCC_CFGR_STOPHSI_Msk,
                    WHAL_STM32WB0_RCC_CFGR_STOPHSI_Msk);
    return WHAL_SUCCESS;
}

/**
 * @brief Lock the RC64MPLL onto HSE and wait for PLL64M.
 *
 * Programs LOCKDET_NSTOP, sets HSIPLLON and polls HSIPLLRDY. Once locked
 * the RC64MPLL source of the fast clock tree is the accurate PLL64M. HSE
 * must already be running.
 *
 * @param cfg     PLL configuration.
 * @param timeout Timeout instance (NULL for unbounded wait).
 *
 * @retval WHAL_SUCCESS    PLL is locked.
 * @retval WHAL_EINVAL     lockDetNstop is out of range.
 * @retval WHAL_ENOTREADY  HSE is not running.
 * @retval WHAL_ETIMEOUT   HSIPLLRDY did not set within the timeout.
 */
static inline whal_Error whal_Stm32wb0_Rcc_EnablePll(
    const whal_Stm32wb0_Rcc_PllCfg *cfg, whal_Timeout *timeout)
{
    if (cfg->lockDetNstop > 7)
        return WHAL_EINVAL;

    if (!(whal_Reg_Read(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG) &
          WHAL_STM32WB0_RCC_CR_HSERDY_Msk))
        return WHAL_ENOTREADY;

    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                    WHAL_STM32WB0_RCC_CR_LOCKDET_NSTOP_Msk,
                    whal_SetBits(WHAL_STM32WB0_RCC_CR_LOCKDET_NSTOP_Msk,
                                 WHAL_STM32WB0_RCC_CR_LOCKDET_NSTOP_Pos,
                                 cfg->lockDetNstop));
    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                    WHAL_STM32WB0_RCC_CR_HSIPLLON_Msk,
                    WHAL_STM32WB0_RCC_CR_HSIPLLON_Msk);
    return whal_Reg_ReadPoll(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                             WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Msk,
                             WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Msk, timeout);
}

/**
 * @brief Release the PLL lock (clears RCC_CR.HSIPLLON).
 *
 * The RC64MPLL source falls back to the free-running HSI.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Stm32wb0_Rcc_DisablePll(void)
{
    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                    WHAL_STM32WB0_RCC_CR_HSIPLLON_Msk, 0);
    return WHAL_SUCCESS;
}

/**
 * @brief Select the fast clock source and SYSCLK frequency.
 *
 * Switching to HSE lowers CLKSYSDIV before setting HSESEL; switching to
 * RC64MPLL sets HSESEL first and then applies CLKSYSDIV, so SYSCLK never
 * requests 64 MHz from the HSE path. Each step polls its status field.
 *
 * Uses the direct RCC_CFGR path, which is not safe while MR_BLE is clocked.
 *
 * @param src     Fast clock tree source.
 * @param div     SYSCLK frequency selection.
 * @param timeout Timeout instance (NULL for unbounded wait).
 *
 * @retval WHAL_SUCCESS  Clock switched.
 * @retval WHAL_EINVAL   64 MHz requested from HSE, or invalid div.
 * @retval WHAL_ETIMEOUT A status field did not update within the timeout.
 */
static inline whal_Error whal_Stm32wb0_Rcc_SetSysClock(
    whal_Stm32wb0_Rcc_SysClockSrc src, whal_Stm32wb0_Rcc_SysClockDiv div,
    whal_Timeout *timeout)
{
    whal_Error err;
    size_t hseSel = (src == WHAL_STM32WB0_RCC_SYSCLK_SRC_HSE);

    if (div > WHAL_STM32WB0_RCC_SYSCLK_1MHZ)
        return WHAL_EINVAL;
    if (hseSel && div == WHAL_STM32WB0_RCC_SYSCLK_64MHZ)
        return WHAL_EINVAL;

    if (hseSel) {
        whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                        WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Msk,
                        whal_SetBits(WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Msk,
                                     WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Pos, div));
        err = whal_Reg_ReadPoll(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                                WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Msk,
                                whal_SetBits(WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Msk,
                                             WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Pos,
                                             div),
                                timeout);
        if (err)
            return err;
    }

    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                    WHAL_STM32WB0_RCC_CFGR_HSESEL_Msk,
                    whal_SetBits(WHAL_STM32WB0_RCC_CFGR_HSESEL_Msk,
                                 WHAL_STM32WB0_RCC_CFGR_HSESEL_Pos, hseSel));
    err = whal_Reg_ReadPoll(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                            WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Msk,
                            whal_SetBits(WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Msk,
                                         WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Pos,
                                         hseSel),
                            timeout);
    if (err || hseSel)
        return err;

    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                    WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Msk,
                    whal_SetBits(WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Msk,
                                 WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_Pos, div));
    return whal_Reg_ReadPoll(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                             WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Msk,
                             whal_SetBits(WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Msk,
                                          WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Pos,
                                          div),
                             timeout);
}

/**
 * @brief Select the slow clock tree source (RCC_CFGR.CLKSLOWSEL).
 *
 * The selected oscillator must already be running. The hardware has no
 * glitch-free switch on this mux.
 *
 * @param src Slow clock source.
 *
 * @retval WHAL_SUCCESS Source selected.
 * @retval WHAL_EINVAL  Invalid source.
 */
static inline whal_Error whal_Stm32wb0_Rcc_SetSlowClock(
    whal_Stm32wb0_Rcc_SlowClockSrc src)
{
    if (src > WHAL_STM32WB0_RCC_SLOWCLK_SRC_16MHZ_DIV512)
        return WHAL_EINVAL;

    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                    WHAL_STM32WB0_RCC_CFGR_CLKSLOWSEL_Msk,
                    whal_SetBits(WHAL_STM32WB0_RCC_CFGR_CLKSLOWSEL_Msk,
                                 WHAL_STM32WB0_RCC_CFGR_CLKSLOWSEL_Pos, src));
    return WHAL_SUCCESS;
}

/**
 * @brief Select the LPUART kernel clock (RCC_CFGR.LPUCLKSEL).
 *
 * @param src LPUART clock source.
 *
 * @retval WHAL_SUCCESS Source selected.
 * @retval WHAL_EINVAL  Invalid source.
 */
static inline whal_Error whal_Stm32wb0_Rcc_SetLpuartClock(
    whal_Stm32wb0_Rcc_LpuartClockSrc src)
{
    if (src > WHAL_STM32WB0_RCC_LPUARTCLK_SRC_LSE)
        return WHAL_EINVAL;

    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                    WHAL_STM32WB0_RCC_CFGR_LPUCLKSEL_Msk,
                    whal_SetBits(WHAL_STM32WB0_RCC_CFGR_LPUCLKSEL_Msk,
                                 WHAL_STM32WB0_RCC_CFGR_LPUCLKSEL_Pos, src));
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
    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, clk->regOffset, clk->enableMask,
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
    whal_Reg_Update(WHAL_STM32WB0_RCC_BASE, clk->regOffset, clk->enableMask,
                    whal_SetBits(clk->enableMask, clk->enablePos, 0));
    return WHAL_SUCCESS;
}

#endif /* WHAL_STM32WB0_RCC_H */
