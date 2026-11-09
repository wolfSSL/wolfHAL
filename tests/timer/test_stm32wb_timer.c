/* test_stm32wb_timer.c
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
#include <wolfHAL/timer/systick.h>
#include "wolfHAL_board.h"
#include "test.h"

#define SYSTICK_CSR         0x00
#define SYSTICK_RVR         0x04
#define SYSTICK_CVR         0x08

#define CSR_ENABLE          (1UL << 0)
#define CSR_TICKINT         (1UL << 1)
#define CSR_CLKSOURCE       (1UL << 2)

#define RVR_RELOAD_Msk      0x00FFFFFFUL

/* Spin budget for one tick to elapse; far longer than a 1 ms tick */
#define TICK_SPIN_MAX       10000000UL

static uintptr_t SysTickBase(void)
{
    return whal_SysTick_Dev.base;
}

static const whal_SysTick_Cfg *SysTickCfg(void)
{
    return (const whal_SysTick_Cfg *)whal_SysTick_Dev.cfg;
}

static int TickAdvances(void)
{
    uint32_t before = g_tick;

    for (volatile size_t i = 0; i < TICK_SPIN_MAX; i++) {
        if (g_tick != before)
            return 1;
    }

    return 0;
}

static void CheckConfig(void)
{
    const whal_SysTick_Cfg *cfg = SysTickCfg();
    size_t csr = whal_Reg32_Read(SysTickBase(), SYSTICK_CSR);

    WHAL_ASSERT_EQ(whal_Reg32_Read(SysTickBase(), SYSTICK_RVR) & RVR_RELOAD_Msk,
                   cfg->cyclesPerTick - 1);
    WHAL_ASSERT_EQ((csr & CSR_CLKSOURCE) != 0,
                   cfg->clkSrc == WHAL_SYSTICK_CLKSRC_SYSCLK);
    WHAL_ASSERT_EQ((csr & CSR_TICKINT) != 0,
                   cfg->tickInt == WHAL_SYSTICK_TICKINT_ENABLED);
    WHAL_ASSERT_EQ(csr & CSR_ENABLE, CSR_ENABLE);
}

/* Board_Init programmed the reload value, clock source and tick interrupt
 * from the board's whal_SysTick_Cfg and started the counter. */
static void Test_SysTick_ConfigReadback(void)
{
    CheckConfig();
}

static void Test_SysTick_StopStart(void)
{
    whal_Error stopErr;
    size_t csr;
    size_t cvr1, cvr2;

    stopErr = whal_Timer_Stop(WHAL_INTERNAL_DEV);
    csr = whal_Reg32_Read(SysTickBase(), SYSTICK_CSR);

    /* A stopped counter holds its value */
    cvr1 = whal_Reg32_Read(SysTickBase(), SYSTICK_CVR);
    for (volatile size_t i = 0; i < 1000; i++) {}
    cvr2 = whal_Reg32_Read(SysTickBase(), SYSTICK_CVR);

    /* Restart before asserting so later tests keep a time base */
    WHAL_ASSERT_EQ(whal_Timer_Start(WHAL_INTERNAL_DEV), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(stopErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(csr & CSR_ENABLE, 0);
    WHAL_ASSERT_EQ(cvr1, cvr2);

    WHAL_ASSERT_EQ(whal_Reg32_Read(SysTickBase(), SYSTICK_CSR) & CSR_ENABLE,
                   CSR_ENABLE);
    WHAL_ASSERT_EQ(TickAdvances(), 1);
}

static void Test_SysTick_Deinit(void)
{
    whal_Error deinitErr;
    size_t csr;
    size_t cvr;

    deinitErr = whal_Timer_Deinit(WHAL_INTERNAL_DEV);
    csr = whal_Reg32_Read(SysTickBase(), SYSTICK_CSR);
    cvr = whal_Reg32_Read(SysTickBase(), SYSTICK_CVR);

    /* Restore the tick before asserting so later tests keep a time base */
    WHAL_ASSERT_EQ(whal_Timer_Init(WHAL_INTERNAL_DEV), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Timer_Start(WHAL_INTERNAL_DEV), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(deinitErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(csr & (CSR_ENABLE | CSR_TICKINT), 0);
    WHAL_ASSERT_EQ(cvr, 0);

    CheckConfig();
    if (g_whalTestCurFailed)
        return;

    WHAL_ASSERT_EQ(TickAdvances(), 1);
}

void whal_Test_Timer_Platform(void)
{
    WHAL_TEST_SUITE_START("timer_platform");
    WHAL_TEST(Test_SysTick_ConfigReadback);
    WHAL_TEST(Test_SysTick_StopStart);
    WHAL_TEST(Test_SysTick_Deinit);
    WHAL_TEST_SUITE_END();
}
