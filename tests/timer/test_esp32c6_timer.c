/* test_esp32c6_timer.c
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
#include <wolfHAL/timer/esp32c6_systimer.h>
#include "wolfHAL_board.h"
#include "test.h"

#define SYSTIMER_CONF_REG               0x0000
#define SYSTIMER_UNIT_WORK_EN_Msk(u)    (1UL << (30 - (u)))
#define SYSTIMER_TARGET_WORK_EN_Msk(t)  (1UL << (24 - (t)))

#define SYSTIMER_UNIT_OP_REG(u)         (0x0004 + 4 * (u))
#define SYSTIMER_UNIT_UPDATE_Msk        (1UL << 30)
#define SYSTIMER_UNIT_VALUE_VALID_Msk   (1UL << 29)
#define SYSTIMER_UNIT_VALUE_HI_REG(u)   (0x0040 + 8 * (u))
#define SYSTIMER_UNIT_VALUE_LO_REG(u)   (0x0044 + 8 * (u))

#define SYSTIMER_TARGET_CONF_REG(t)     (0x0034 + 4 * (t))
#define SYSTIMER_TARGET_PERIOD_Msk      0x03FFFFFFUL
#define SYSTIMER_TARGET_PERIOD_MODE_Msk (1UL << 30)
#define SYSTIMER_TARGET_UNIT_SEL_Msk    (1UL << 31)

#define SYSTIMER_INT_ENA_REG            0x0064
#define SYSTIMER_INT_RAW_REG            0x0068
#define SYSTIMER_INT_CLR_REG            0x006C
#define SYSTIMER_INT_Msk(t)             (1UL << (t))

#define SYSTIMER_TICKS_PER_MS           16000
#define SYSTIMER_TARGET_MAX             0x000FFFFFFFFFFFFFULL

#define TICK_SPIN_MAX                   10000000UL
#define VALID_SPIN_MAX                  100000UL

static size_t SystimerBase(void)
{
    return g_whalTimer.base;
}

static const whal_Esp32c6_Systimer_Cfg *SystimerCfg(void)
{
    return (const whal_Esp32c6_Systimer_Cfg *)g_whalTimer.cfg;
}

static uint64_t ReadCount(size_t unit)
{
    size_t base = SystimerBase();
    uint32_t hi;
    uint32_t lo;

    whal_Reg_Write(base, SYSTIMER_UNIT_OP_REG(unit), SYSTIMER_UNIT_UPDATE_Msk);
    for (volatile size_t i = 0; i < VALID_SPIN_MAX; i++) {
        if (whal_Reg_Read(base, SYSTIMER_UNIT_OP_REG(unit)) &
            SYSTIMER_UNIT_VALUE_VALID_Msk)
            break;
    }

    hi = whal_Reg_Read(base, SYSTIMER_UNIT_VALUE_HI_REG(unit));
    lo = whal_Reg_Read(base, SYSTIMER_UNIT_VALUE_LO_REG(unit));

    return ((uint64_t)hi << 32) | lo;
}

static void SpinCounts(size_t unit, uint64_t counts)
{
    uint64_t start = ReadCount(unit);

    while (ReadCount(unit) - start < counts) {}
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

static void RestoreTick(void)
{
    whal_Timer_Init(&g_whalTimer);
    whal_Timer_Start(&g_whalTimer);
}

static void CheckConfig(void)
{
    const whal_Esp32c6_Systimer_Cfg *cfg = SystimerCfg();
    size_t base = SystimerBase();
    size_t conf = whal_Reg_Read(base, SYSTIMER_TARGET_CONF_REG(cfg->comp));

    WHAL_ASSERT_EQ(conf & SYSTIMER_TARGET_PERIOD_Msk, cfg->period);
    WHAL_ASSERT_EQ(conf & SYSTIMER_TARGET_PERIOD_MODE_Msk,
                   SYSTIMER_TARGET_PERIOD_MODE_Msk);
    WHAL_ASSERT_EQ((conf & SYSTIMER_TARGET_UNIT_SEL_Msk) != 0, cfg->unit);
    WHAL_ASSERT_EQ(whal_Reg_Read(base, SYSTIMER_CONF_REG) &
                   (SYSTIMER_UNIT_WORK_EN_Msk(cfg->unit) |
                    SYSTIMER_TARGET_WORK_EN_Msk(cfg->comp)),
                   SYSTIMER_UNIT_WORK_EN_Msk(cfg->unit) |
                   SYSTIMER_TARGET_WORK_EN_Msk(cfg->comp));
    WHAL_ASSERT_EQ(whal_Reg_Read(base, SYSTIMER_INT_ENA_REG) &
                   SYSTIMER_INT_Msk(cfg->comp),
                   SYSTIMER_INT_Msk(cfg->comp));
}

static void Test_Systimer_ConfigReadback(void)
{
    CheckConfig();
}

static void Test_Systimer_StopStart(void)
{
    const whal_Esp32c6_Systimer_Cfg *cfg = SystimerCfg();
    whal_Error stopErr;
    whal_Error startErr;
    size_t conf;
    uint32_t tickBefore;
    uint32_t tickAfter;
    int advanced;

    stopErr = whal_Timer_Stop(&g_whalTimer);
    conf = whal_Reg_Read(SystimerBase(), SYSTIMER_CONF_REG);

    tickBefore = g_tick;
    SpinCounts(cfg->unit, 5 * SYSTIMER_TICKS_PER_MS);
    tickAfter = g_tick;

    startErr = whal_Timer_Start(&g_whalTimer);
    advanced = TickAdvances();
    if (!advanced)
        RestoreTick();

    WHAL_ASSERT_EQ(stopErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(startErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(conf & SYSTIMER_TARGET_WORK_EN_Msk(cfg->comp), 0);
    WHAL_ASSERT_EQ(conf & SYSTIMER_UNIT_WORK_EN_Msk(cfg->unit),
                   SYSTIMER_UNIT_WORK_EN_Msk(cfg->unit));
    WHAL_ASSERT_EQ(tickAfter, tickBefore);
    WHAL_ASSERT_EQ(advanced, 1);
}

static void Test_Systimer_Deinit(void)
{
    const whal_Esp32c6_Systimer_Cfg *cfg = SystimerCfg();
    whal_Error deinitErr;
    size_t conf;
    size_t intEna;

    deinitErr = whal_Timer_Deinit(&g_whalTimer);
    conf = whal_Reg_Read(SystimerBase(), SYSTIMER_CONF_REG);
    intEna = whal_Reg_Read(SystimerBase(), SYSTIMER_INT_ENA_REG);

    WHAL_ASSERT_EQ(whal_Timer_Init(&g_whalTimer), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Timer_Start(&g_whalTimer), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(deinitErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(conf & SYSTIMER_TARGET_WORK_EN_Msk(cfg->comp), 0);
    WHAL_ASSERT_EQ(intEna & SYSTIMER_INT_Msk(cfg->comp), 0);

    CheckConfig();
    if (g_whalTestCurFailed)
        return;

    WHAL_ASSERT_EQ(TickAdvances(), 1);
}

static void Test_Systimer_BadCfg(void)
{
    whal_Esp32c6_Systimer_Cfg badUnit = {
        .mode = WHAL_ESP32C6_SYSTIMER_PERIODIC,
        .unit = (whal_Esp32c6_Systimer_Unit)(WHAL_ESP32C6_SYSTIMER_UNIT1 + 1),
        .comp = WHAL_ESP32C6_SYSTIMER_COMP1,
        .period = SYSTIMER_TICKS_PER_MS,
    };
    whal_Esp32c6_Systimer_Cfg badComp = {
        .mode = WHAL_ESP32C6_SYSTIMER_PERIODIC,
        .unit = WHAL_ESP32C6_SYSTIMER_UNIT0,
        .comp = (whal_Esp32c6_Systimer_Comp)(WHAL_ESP32C6_SYSTIMER_COMP2 + 1),
        .period = SYSTIMER_TICKS_PER_MS,
    };
    whal_Timer unitDev = {
        .base = SystimerBase(),
        .driver = WHAL_ESP32C6_SYSTIMER_DRIVER,
        .cfg = &badUnit,
    };
    whal_Timer compDev = {
        .base = SystimerBase(),
        .driver = WHAL_ESP32C6_SYSTIMER_DRIVER,
        .cfg = &badComp,
    };

    WHAL_ASSERT_EQ(whal_Timer_Init(&unitDev), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Timer_Start(&unitDev), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Timer_Reset(&unitDev), WHAL_EINVAL);

    WHAL_ASSERT_EQ(whal_Timer_Init(&compDev), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Timer_Deinit(&compDev), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Timer_Start(&compDev), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Timer_Stop(&compDev), WHAL_EINVAL);
}

static void Test_Systimer_OneShot(void)
{
    whal_Esp32c6_Systimer_Cfg cfg = {
        .mode = WHAL_ESP32C6_SYSTIMER_ONESHOT,
        .unit = WHAL_ESP32C6_SYSTIMER_UNIT0,
        .comp = WHAL_ESP32C6_SYSTIMER_COMP1,
    };
    whal_Timer dev = {
        .base = SystimerBase(),
        .driver = WHAL_ESP32C6_SYSTIMER_DRIVER,
        .cfg = &cfg,
    };
    size_t base = SystimerBase();
    size_t intMsk = SYSTIMER_INT_Msk(WHAL_ESP32C6_SYSTIMER_COMP1);
    whal_Error initErr;
    whal_Error startErr;
    whal_Error deinitErr;
    uint32_t start;
    uint32_t elapsed;
    int fired = 0;
    int refired;
    size_t conf;
    size_t intEna;

    cfg.compValue = ReadCount(cfg.unit) + 10 * SYSTIMER_TICKS_PER_MS;

    initErr = whal_Timer_Init(&dev);
    whal_Reg_Write(base, SYSTIMER_INT_CLR_REG, intMsk);
    startErr = whal_Timer_Start(&dev);

    start = g_tick;
    while (g_tick - start < 100) {
        if (whal_Reg_Read(base, SYSTIMER_INT_RAW_REG) & intMsk) {
            fired = 1;
            break;
        }
    }
    elapsed = g_tick - start;

    whal_Reg_Write(base, SYSTIMER_INT_CLR_REG, intMsk);
    Board_WaitMs(30);
    refired = (whal_Reg_Read(base, SYSTIMER_INT_RAW_REG) & intMsk) != 0;

    deinitErr = whal_Timer_Deinit(&dev);
    whal_Reg_Write(base, SYSTIMER_INT_CLR_REG, intMsk);
    conf = whal_Reg_Read(base, SYSTIMER_CONF_REG);
    intEna = whal_Reg_Read(base, SYSTIMER_INT_ENA_REG);

    WHAL_ASSERT_EQ(initErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(startErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(deinitErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(fired, 1);
    WHAL_ASSERT_EQ(elapsed >= 9, 1);
    WHAL_ASSERT_EQ(refired, 0);
    WHAL_ASSERT_EQ(conf & SYSTIMER_TARGET_WORK_EN_Msk(cfg.comp), 0);
    WHAL_ASSERT_EQ(intEna & intMsk, 0);
}

static void Test_Systimer_Reset(void)
{
    whal_Esp32c6_Systimer_Cfg cfg = {
        .mode = WHAL_ESP32C6_SYSTIMER_ONESHOT,
        .unit = WHAL_ESP32C6_SYSTIMER_UNIT1,
        .comp = WHAL_ESP32C6_SYSTIMER_COMP2,
        .compValue = SYSTIMER_TARGET_MAX,
    };
    whal_Timer dev = {
        .base = SystimerBase(),
        .driver = WHAL_ESP32C6_SYSTIMER_DRIVER,
        .cfg = &cfg,
    };
    size_t base = SystimerBase();
    whal_Error initErr;
    whal_Error startErr;
    whal_Error resetErr;
    whal_Error deinitErr;
    uint64_t before;
    uint64_t after;

    initErr = whal_Timer_Init(&dev);
    startErr = whal_Timer_Start(&dev);
    Board_WaitMs(5);

    before = ReadCount(cfg.unit);
    resetErr = whal_Timer_Reset(&dev);
    after = ReadCount(cfg.unit);

    deinitErr = whal_Timer_Deinit(&dev);
    whal_Reg_Update(base, SYSTIMER_CONF_REG,
                    SYSTIMER_UNIT_WORK_EN_Msk(cfg.unit), 0);
    whal_Reg_Write(base, SYSTIMER_INT_CLR_REG, SYSTIMER_INT_Msk(cfg.comp));

    WHAL_ASSERT_EQ(initErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(startErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(resetErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(deinitErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(before >= 4 * SYSTIMER_TICKS_PER_MS, 1);
    WHAL_ASSERT_EQ(after < SYSTIMER_TICKS_PER_MS, 1);
}

void whal_Test_Timer_Platform(void)
{
    WHAL_TEST_SUITE_START("timer_platform");
    WHAL_TEST(Test_Systimer_ConfigReadback);
    WHAL_TEST(Test_Systimer_StopStart);
    WHAL_TEST(Test_Systimer_Deinit);
    WHAL_TEST(Test_Systimer_BadCfg);
    WHAL_TEST(Test_Systimer_OneShot);
    WHAL_TEST(Test_Systimer_Reset);
    WHAL_TEST_SUITE_END();
}
