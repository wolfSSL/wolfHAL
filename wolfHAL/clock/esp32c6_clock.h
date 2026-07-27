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

#include <wolfHAL/error.h>

/*
 * @file esp32c6_clock.h
 * @brief ESP32-C6 clock system driver (stub).
 *
 * As with other wolfHAL clock drivers, the ESP32-C6 clock tree has no generic
 * abstraction: a board brings it up imperatively. The ESP32-C6 uses the PCR
 * (Power, Clock and Reset) registers for per-peripheral clock enable and reset,
 * plus XTAL/PLL selection for the CPU clock. The full API (PLL/XTAL setup and a
 * PeriphClk enable/disable pair) is still to be designed; only Init is
 * scaffolded here.
 */

/*
 * @brief Initialize the ESP32-C6 clock tree.
 *
 * @retval WHAL_SUCCESS Clock configured.
 * @retval WHAL_ENOTSUP Not yet implemented.
 */
whal_Error whal_Esp32c6_Clock_Init(void);

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

static inline void whal_Esp32c6_Clock_SetSysClk(const whal_Esp32c6_Clock_HpSysClk sysClk)
{
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_SYSCLK_CONF_REG, 
                    WHAL_PCR_SOC_CLK_SEL_Msk, sysClk);
}

static inline void whal_Esp32c6_Clock_ConfigureCpuFreq(const whal_Esp32c6_Clock_CpuLsDiv cpuLsDiv,
                                                       const whal_Esp32c6_Clock_CpuHsDiv cpuHsDiv,
                                                       const whal_Esp32c6_Clock_Hs120MForce hs120MForce)
{
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_CPU_FREQ_CONF_REG, 
                    WHAL_PCR_LS_DIV_Msk | WHAL_PCR_HS_DIV_Msk |WHAL_PCR_HS_120M_FORCE_Msk,
                    whal_SetBits(WHAL_PCR_LS_DIV_Msk, WHAL_PCR_LS_DIV_Pos, cpuLsDiv) |
                    whal_SetBits(WHAL_PCR_HS_DIV_Msk, WHAL_PCR_HS_DIV_Pos, cpuHsDiv) |
                    whal_SetBits(WHAL_PCR_HS_120M_FORCE_Msk, WHAL_PCR_HS_120M_FORCE_Pos, hs120MForce));
}

/* UART0 clock gates (PCR), TRM Reg 8.1 / 8.2 */
#define WHAL_PCR_UART0_CONF_REG       0x0000
#define WHAL_PCR_UART0_CLK_EN_Pos     0
#define WHAL_PCR_UART0_CLK_EN_Msk     (1UL << WHAL_PCR_UART0_CLK_EN_Pos)
#define WHAL_PCR_UART0_RST_EN_Pos     1
#define WHAL_PCR_UART0_RST_EN_Msk     (1UL << WHAL_PCR_UART0_RST_EN_Pos)

#define WHAL_PCR_UART0_SCLK_CONF_REG  0x0004
#define WHAL_PCR_UART0_SCLK_EN_Pos    22
#define WHAL_PCR_UART0_SCLK_EN_Msk    (1UL << WHAL_PCR_UART0_SCLK_EN_Pos)

static inline void whal_Esp32c6_Clock_EnableUart0Clk(void)
{
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_UART0_CONF_REG,
                    WHAL_PCR_UART0_CLK_EN_Msk | WHAL_PCR_UART0_RST_EN_Msk,
                    whal_SetBits(WHAL_PCR_UART0_CLK_EN_Msk, WHAL_PCR_UART0_CLK_EN_Pos, 1) |
                    whal_SetBits(WHAL_PCR_UART0_RST_EN_Msk, WHAL_PCR_UART0_RST_EN_Pos, 0));
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_UART0_SCLK_CONF_REG,
                    WHAL_PCR_UART0_SCLK_EN_Msk,
                    whal_SetBits(WHAL_PCR_UART0_SCLK_EN_Msk, WHAL_PCR_UART0_SCLK_EN_Pos, 1));
}

static inline void whal_Esp32c6_Clock_DisableUart0Clk(void)
{
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_UART0_SCLK_CONF_REG,
                    WHAL_PCR_UART0_SCLK_EN_Msk,
                    whal_SetBits(WHAL_PCR_UART0_SCLK_EN_Msk, WHAL_PCR_UART0_SCLK_EN_Pos, 0));
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_UART0_CONF_REG,
                    WHAL_PCR_UART0_CLK_EN_Msk,
                    whal_SetBits(WHAL_PCR_UART0_CLK_EN_Msk, WHAL_PCR_UART0_CLK_EN_Pos, 0));
}

/* IO MUX clock gates (PCR), TRM Reg 8.55 / 8.56 */
#define WHAL_PCR_IOMUX_CONF_REG          0x00E8
#define WHAL_PCR_IOMUX_CLK_EN_Pos        0
#define WHAL_PCR_IOMUX_CLK_EN_Msk        (1UL << WHAL_PCR_IOMUX_CLK_EN_Pos)
#define WHAL_PCR_IOMUX_RST_EN_Pos        1
#define WHAL_PCR_IOMUX_RST_EN_Msk        (1UL << WHAL_PCR_IOMUX_RST_EN_Pos)

#define WHAL_PCR_IOMUX_CLK_CONF_REG      0x00EC
#define WHAL_PCR_IOMUX_FUNC_CLK_EN_Pos   22
#define WHAL_PCR_IOMUX_FUNC_CLK_EN_Msk   (1UL << WHAL_PCR_IOMUX_FUNC_CLK_EN_Pos)

static inline void whal_Esp32c6_Clock_EnableIomuxClk(void)
{
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_IOMUX_CONF_REG,
                    WHAL_PCR_IOMUX_CLK_EN_Msk | WHAL_PCR_IOMUX_RST_EN_Msk,
                    whal_SetBits(WHAL_PCR_IOMUX_CLK_EN_Msk, WHAL_PCR_IOMUX_CLK_EN_Pos, 1) |
                    whal_SetBits(WHAL_PCR_IOMUX_RST_EN_Msk, WHAL_PCR_IOMUX_RST_EN_Pos, 0));
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_IOMUX_CLK_CONF_REG,
                    WHAL_PCR_IOMUX_FUNC_CLK_EN_Msk,
                    whal_SetBits(WHAL_PCR_IOMUX_FUNC_CLK_EN_Msk, WHAL_PCR_IOMUX_FUNC_CLK_EN_Pos, 1));
}

static inline void whal_Esp32c6_Clock_DisableIomuxClk(void)
{
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_IOMUX_CLK_CONF_REG,
                    WHAL_PCR_IOMUX_FUNC_CLK_EN_Msk,
                    whal_SetBits(WHAL_PCR_IOMUX_FUNC_CLK_EN_Msk, WHAL_PCR_IOMUX_FUNC_CLK_EN_Pos, 0));
    whal_Reg_Update(WHAL_ESP32C6_CLOCK_BASE, WHAL_PCR_IOMUX_CONF_REG,
                    WHAL_PCR_IOMUX_CLK_EN_Msk,
                    whal_SetBits(WHAL_PCR_IOMUX_CLK_EN_Msk, WHAL_PCR_IOMUX_CLK_EN_Pos, 0));
}

#endif /* WHAL_ESP32C6_CLOCK_H */
