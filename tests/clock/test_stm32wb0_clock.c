/* test_stm32wb0_clock.c
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

#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/clock/stm32wb0_rcc.h>
#include <wolfHAL/bitops.h>
#include "wolfHAL_board.h"
#include "test.h"

static void Test_Clock_EnableDisable(void)
{
    whal_Stm32wb0_Rcc_PeriphClk testClk = { WHAL_STM32WB09_CRC_CLOCK };
    size_t origVal = 0;
    size_t val = 0;

    whal_Reg_Get(WHAL_STM32WB0_RCC_BASE, testClk.regOffset, testClk.enableMask,
                 testClk.enablePos, &origVal);

    WHAL_ASSERT_EQ(whal_Stm32wb0_Rcc_EnablePeriphClk(&testClk), WHAL_SUCCESS);
    whal_Reg_Get(WHAL_STM32WB0_RCC_BASE, testClk.regOffset, testClk.enableMask,
                 testClk.enablePos, &val);
    WHAL_ASSERT_EQ(val, 1);

    WHAL_ASSERT_EQ(whal_Stm32wb0_Rcc_DisablePeriphClk(&testClk), WHAL_SUCCESS);
    whal_Reg_Get(WHAL_STM32WB0_RCC_BASE, testClk.regOffset, testClk.enableMask,
                 testClk.enablePos, &val);
    WHAL_ASSERT_EQ(val, 0);

    if (origVal)
        whal_Stm32wb0_Rcc_EnablePeriphClk(&testClk);
}

/* Board_Init leaves SYSCLK at 64 MHz on the PLL-locked RC64MPLL tree. */
static void Test_Clock_Pll64mActive(void)
{
    size_t val = 0;

    whal_Reg_Get(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                 WHAL_STM32WB0_RCC_CR_HSERDY_Msk,
                 WHAL_STM32WB0_RCC_CR_HSERDY_Pos, &val);
    WHAL_ASSERT_EQ(val, 1);

    whal_Reg_Get(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CR_REG,
                 WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Msk,
                 WHAL_STM32WB0_RCC_CR_HSIPLLRDY_Pos, &val);
    WHAL_ASSERT_EQ(val, 1);

    whal_Reg_Get(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                 WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Msk,
                 WHAL_STM32WB0_RCC_CFGR_HSESEL_STATUS_Pos, &val);
    WHAL_ASSERT_EQ(val, WHAL_STM32WB0_RCC_SYSCLK_SRC_RC64MPLL);

    whal_Reg_Get(WHAL_STM32WB0_RCC_BASE, WHAL_STM32WB0_RCC_CFGR_REG,
                 WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Msk,
                 WHAL_STM32WB0_RCC_CFGR_CLKSYSDIV_STATUS_Pos, &val);
    WHAL_ASSERT_EQ(val, WHAL_STM32WB0_RCC_SYSCLK_64MHZ);
}

static void Test_Clock_RejectHse64Mhz(void)
{
    WHAL_ASSERT_EQ(whal_Stm32wb0_Rcc_SetSysClock(
                       WHAL_STM32WB0_RCC_SYSCLK_SRC_HSE,
                       WHAL_STM32WB0_RCC_SYSCLK_64MHZ, &g_whalTimeout),
                   WHAL_EINVAL);
}

static void Test_Clock_PllBadLockDet(void)
{
    WHAL_ASSERT_EQ(whal_Stm32wb0_Rcc_EnablePll(
                       &(whal_Stm32wb0_Rcc_PllCfg){.lockDetNstop = 8},
                       &g_whalTimeout),
                   WHAL_EINVAL);
}

void whal_Test_Clock_Platform(void)
{
    WHAL_TEST(Test_Clock_EnableDisable);
    WHAL_TEST(Test_Clock_Pll64mActive);
    WHAL_TEST(Test_Clock_RejectHse64Mhz);
    WHAL_TEST(Test_Clock_PllBadLockDet);
}
