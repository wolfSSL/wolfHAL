/* esp32c6_clock.h
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

#ifndef WHAL_ESP32C6_CLOCK_H
#define WHAL_ESP32C6_CLOCK_H

#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * @file esp32c6_clock.h
 * @brief ESP32-C6 clock system driver.
 *
 * As with other wolfHAL clock drivers, the ESP32-C6 clock tree has no generic
 * abstraction: a board brings it up imperatively. The PCR (Power, Clock and
 * Reset) block holds the CPU clock source and divider selection, plus a
 * per-peripheral bus clock enable, reset, and function clock enable.
 */

#define WHAL_ESP32C6_CLOCK_BASE   0x60096000

/* CPU clock source select (PCR), TRM Reg 8.63.
 * SOC_CLK_SEL[17:16]: 0 = XTAL, 1 = PLL, 2 = RC_FAST. */
#define WHAL_PCR_SYSCLK_CONF_REG         0x0110
#define WHAL_PCR_SOC_CLK_SEL_Pos         16
#define WHAL_PCR_SOC_CLK_SEL_Msk         (WHAL_BITMASK(2) << WHAL_PCR_SOC_CLK_SEL_Pos)

/* CPU frequency config (PCR), TRM Reg 8.65. The divider fields hold
 * (divide factor - 1); HS_120M_FORCE is only valid when HS_DIV is /1. */
#define WHAL_PCR_CPU_FREQ_CONF_REG       0x0118
#define WHAL_PCR_LS_DIV_Pos              0
#define WHAL_PCR_LS_DIV_Msk              (WHAL_BITMASK(8) << WHAL_PCR_LS_DIV_Pos)
#define WHAL_PCR_HS_DIV_Pos              8
#define WHAL_PCR_HS_DIV_Msk              (WHAL_BITMASK(8) << WHAL_PCR_HS_DIV_Pos)
#define WHAL_PCR_HS_120M_FORCE_Pos       16
#define WHAL_PCR_HS_120M_FORCE_Msk       (1UL << WHAL_PCR_HS_120M_FORCE_Pos)

/*
 * @brief HP system (CPU root) clock source (PCR_SOC_CLK_SEL, TRM Reg 8.63).
 */
typedef enum {
    WHAL_ESP32C6_HP_SYS_CLK_XTAL    = 0,
    WHAL_ESP32C6_HP_SYS_CLK_PLL     = 1,
    WHAL_ESP32C6_HP_SYS_CLK_RC_FAST = 2,
} whal_Esp32c6_Clock_HpSysClk;

/*
 * @brief CPU_CLK divider from a low-speed source (PCR_CPU_LS_DIV_NUM).
 * Selected when the HP system clock is XTAL or RC_FAST. Enum value is the
 * register field (divide factor - 1).
 */
typedef enum {
    WHAL_ESP32C6_CPU_LS_DIV_1  = 0,
    WHAL_ESP32C6_CPU_LS_DIV_2  = 1,
    WHAL_ESP32C6_CPU_LS_DIV_4  = 3,
    WHAL_ESP32C6_CPU_LS_DIV_8  = 7,
    WHAL_ESP32C6_CPU_LS_DIV_16 = 15,
    WHAL_ESP32C6_CPU_LS_DIV_32 = 31,
} whal_Esp32c6_Clock_CpuLsDiv;

/*
 * @brief CPU_CLK divider from a high-speed source (PCR_CPU_HS_DIV_NUM).
 * Selected when the HP system clock is PLL. Enum value is the register field
 * (divide factor - 1).
 */
typedef enum {
    WHAL_ESP32C6_CPU_HS_DIV_1 = 0,
    WHAL_ESP32C6_CPU_HS_DIV_2 = 1,
    WHAL_ESP32C6_CPU_HS_DIV_4 = 3,
} whal_Esp32c6_Clock_CpuHsDiv;

/*
 * @brief Force CPU_CLK to 120 MHz (PCR_CPU_HS_120M_FORCE). Only valid when the
 * HS divider is /1 and CPU_CLK is derived from the SPLL.
 */
typedef enum {
    WHAL_ESP32C6_CPU_HS_120M_FORCE_DISABLE = 0,
    WHAL_ESP32C6_CPU_HS_120M_FORCE_ENABLE  = 1,
} whal_Esp32c6_Clock_Hs120MForce;

/*
 * @brief Peripheral clock descriptor (PCR bus clock, reset, and function clock).
 *
 * confReg holds the bus clock enable and reset bits. funcClkReg holds the
 * function clock enable; a funcClkEnMsk of 0 means the peripheral has none.
 */
typedef struct {
    size_t confReg;
    size_t clkEnMsk;
    size_t rstEnMsk;
    size_t funcClkReg;
    size_t funcClkEnMsk;
} whal_Esp32c6_Clock_PeriphClk;

/*
 * @brief Select the HP system (CPU root) clock source.
 *
 * @param sysClk Clock source to switch to.
 */
static inline void whal_Esp32c6_Clock_SetSysClk(const whal_Esp32c6_Clock_HpSysClk sysClk)
{
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_SYSCLK_CONF_REG,
                    WHAL_PCR_SOC_CLK_SEL_Msk,
                    whal_SetBits(WHAL_PCR_SOC_CLK_SEL_Msk, WHAL_PCR_SOC_CLK_SEL_Pos, sysClk));
}

/*
 * @brief Configure the CPU_CLK dividers.
 *
 * @param cpuLsDiv    Divider applied when the source is XTAL or RC_FAST.
 * @param cpuHsDiv    Divider applied when the source is PLL.
 * @param hs120MForce Force CPU_CLK to 120 MHz from the PLL.
 */
static inline void whal_Esp32c6_Clock_ConfigureCpuFreq(const whal_Esp32c6_Clock_CpuLsDiv cpuLsDiv,
                                                       const whal_Esp32c6_Clock_CpuHsDiv cpuHsDiv,
                                                       const whal_Esp32c6_Clock_Hs120MForce hs120MForce)
{
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_CPU_FREQ_CONF_REG,
                    WHAL_PCR_LS_DIV_Msk | WHAL_PCR_HS_DIV_Msk | WHAL_PCR_HS_120M_FORCE_Msk,
                    whal_SetBits(WHAL_PCR_LS_DIV_Msk, WHAL_PCR_LS_DIV_Pos, cpuLsDiv) |
                    whal_SetBits(WHAL_PCR_HS_DIV_Msk, WHAL_PCR_HS_DIV_Pos, cpuHsDiv) |
                    whal_SetBits(WHAL_PCR_HS_120M_FORCE_Msk, WHAL_PCR_HS_120M_FORCE_Pos, hs120MForce));
}

/*
 * @brief Enable a peripheral's bus clock, release its reset, and enable its
 * function clock.
 *
 * @param clk Peripheral clock descriptor.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Esp32c6_Clock_EnablePeriphClk(
    const whal_Esp32c6_Clock_PeriphClk *clk)
{
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, clk->confReg,
                    clk->clkEnMsk | clk->rstEnMsk, clk->clkEnMsk);
    if (clk->funcClkEnMsk) {
        whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, clk->funcClkReg,
                        clk->funcClkEnMsk, clk->funcClkEnMsk);
    }
    return WHAL_SUCCESS;
}

/*
 * @brief Disable a peripheral's function clock and bus clock.
 *
 * @param clk Peripheral clock descriptor.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Esp32c6_Clock_DisablePeriphClk(
    const whal_Esp32c6_Clock_PeriphClk *clk)
{
    if (clk->funcClkEnMsk) {
        whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, clk->funcClkReg,
                        clk->funcClkEnMsk, 0);
    }
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, clk->confReg, clk->clkEnMsk, 0);
    return WHAL_SUCCESS;
}

#endif /* WHAL_ESP32C6_CLOCK_H */
