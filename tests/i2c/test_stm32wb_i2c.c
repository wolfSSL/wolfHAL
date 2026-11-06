/* test_stm32wb_i2c.c
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
#include <wolfHAL/i2c/stm32wb_i2c.h>
#include "wolfHAL_board.h"
#include "test.h"

/* StartCom only programs TIMINGR and CR2; nothing is driven on the bus, so
 * no target device is needed. */

#define I2C_CR2         0x04
#define I2C_TIMINGR     0x10

#define CR2_SADD_Msk    0x3FFu
#define CR2_ADD10_Msk   (1UL << 11)

/* I2C-bus specification (UM10204) minimum SCL low and high times */
#define SM_TLOW_NS      4700
#define SM_THIGH_NS     4000
#define FM_TLOW_NS      1300
#define FM_THIGH_NS      600
#define FMP_TLOW_NS      500
#define FMP_THIGH_NS     260

typedef struct {
    uint32_t presc;
    uint32_t scldel;
    uint32_t sdadel;
    uint32_t sclh;
    uint32_t scll;
} Timing;

static uintptr_t I2cBase(void)
{
    return BOARD_I2C_DEV->base;
}

static uint32_t I2cPclk(void)
{
    return ((whal_Stm32wb_I2c_Cfg *)BOARD_I2C_DEV->cfg)->pclk;
}

static Timing DecodeTimingr(uint32_t t)
{
    Timing d;

    d.presc  = (t >> 28) & 0xF;
    d.scldel = (t >> 20) & 0xF;
    d.sdadel = (t >> 16) & 0xF;
    d.sclh   = (t >> 8) & 0xFF;
    d.scll   = t & 0xFF;

    return d;
}

/* Duration of n prescaled I2C clock ticks, in nanoseconds (pclk is a whole
 * number of MHz) */
static uint32_t TicksNs(uint32_t presc, uint32_t n)
{
    return (presc + 1) * n * 1000u / (I2cPclk() / 1000000u);
}

static void StartAndRead(uint32_t freq, Timing *t)
{
    whal_I2c_ComCfg com = {
        .freq = freq,
        .addr = 0x50,
        .addrSz = 7,
    };

    WHAL_ASSERT_EQ(whal_I2c_StartCom(BOARD_I2C_DEV, &com), WHAL_SUCCESS);
    *t = DecodeTimingr(whal_Reg32_Read(I2cBase(), I2C_TIMINGR));
    WHAL_ASSERT_EQ(whal_I2c_EndCom(BOARD_I2C_DEV), WHAL_SUCCESS);
}

static void CheckMode(uint32_t freq, uint32_t tLowMin, uint32_t tHighMin,
                      uint32_t sdadel, uint32_t scldel, uint32_t *periodNs)
{
    Timing t = {0};

    StartAndRead(freq, &t);
    if (g_whalTestCurFailed)
        return;

    WHAL_ASSERT_EQ(t.sdadel, sdadel);
    WHAL_ASSERT_EQ(t.scldel, scldel);

    WHAL_ASSERT_EQ(TicksNs(t.presc, t.scll + 1) >= tLowMin, 1);
    WHAL_ASSERT_EQ(TicksNs(t.presc, t.sclh + 1) >= tHighMin, 1);

    *periodNs = TicksNs(t.presc, t.scll + 1 + t.sclh + 1);
}

static void Test_Stm32wb_I2c_Timing(void)
{
    uint32_t p10k = 0, p100k = 0, p400k = 0, p1m = 0;

    CheckMode(10000, SM_TLOW_NS, SM_THIGH_NS, 2, 4, &p10k);
    if (g_whalTestCurFailed)
        return;
    CheckMode(100000, SM_TLOW_NS, SM_THIGH_NS, 2, 4, &p100k);
    if (g_whalTestCurFailed)
        return;
    CheckMode(400000, FM_TLOW_NS, FM_THIGH_NS, 1, 3, &p400k);
    if (g_whalTestCurFailed)
        return;
    CheckMode(1000000, FMP_TLOW_NS, FMP_THIGH_NS, 0, 1, &p1m);
    if (g_whalTestCurFailed)
        return;

    /* Faster requests give shorter SCL periods */
    WHAL_ASSERT_EQ(p1m < p400k, 1);
    WHAL_ASSERT_EQ(p400k < p100k, 1);
    WHAL_ASSERT_EQ(p100k < p10k, 1);

    /* A tenth of the frequency within a mode gives about ten times the
     * period (within 5%) */
    WHAL_ASSERT_EQ(p10k >= p100k * 95 / 10, 1);
    WHAL_ASSERT_EQ(p10k <= p100k * 105 / 10, 1);
}

static void Test_Stm32wb_I2c_Address(void)
{
    whal_I2c_ComCfg com = {
        .freq = 100000,
        .addr = 0x50,
        .addrSz = 7,
    };
    size_t cr2;

    /* 7-bit addresses occupy SADD[7:1] */
    WHAL_ASSERT_EQ(whal_I2c_StartCom(BOARD_I2C_DEV, &com), WHAL_SUCCESS);
    cr2 = whal_Reg32_Read(I2cBase(), I2C_CR2);
    WHAL_ASSERT_EQ(cr2 & CR2_SADD_Msk, 0x50 << 1);
    WHAL_ASSERT_EQ(cr2 & CR2_ADD10_Msk, 0);
    WHAL_ASSERT_EQ(whal_I2c_EndCom(BOARD_I2C_DEV), WHAL_SUCCESS);

    /* Highest 7-bit address */
    com.addr = 0x7F;
    WHAL_ASSERT_EQ(whal_I2c_StartCom(BOARD_I2C_DEV, &com), WHAL_SUCCESS);
    cr2 = whal_Reg32_Read(I2cBase(), I2C_CR2);
    WHAL_ASSERT_EQ(cr2 & CR2_SADD_Msk, 0x7F << 1);
    WHAL_ASSERT_EQ(whal_I2c_EndCom(BOARD_I2C_DEV), WHAL_SUCCESS);

    /* 10-bit addresses occupy SADD[9:0] with ADD10 set */
    com.addr = 0x2A5;
    com.addrSz = 10;
    WHAL_ASSERT_EQ(whal_I2c_StartCom(BOARD_I2C_DEV, &com), WHAL_SUCCESS);
    cr2 = whal_Reg32_Read(I2cBase(), I2C_CR2);
    WHAL_ASSERT_EQ(cr2 & CR2_SADD_Msk, 0x2A5);
    WHAL_ASSERT_EQ(cr2 & CR2_ADD10_Msk, CR2_ADD10_Msk);

    /* EndCom clears the address and addressing mode */
    WHAL_ASSERT_EQ(whal_I2c_EndCom(BOARD_I2C_DEV), WHAL_SUCCESS);
    cr2 = whal_Reg32_Read(I2cBase(), I2C_CR2);
    WHAL_ASSERT_EQ(cr2 & (CR2_SADD_Msk | CR2_ADD10_Msk), 0);
}

static void Test_Stm32wb_I2c_Reject(void)
{
    whal_I2c_ComCfg com = {
        .freq = 1000001,
        .addr = 0x50,
        .addrSz = 7,
    };

    WHAL_ASSERT_EQ(whal_I2c_StartCom(BOARD_I2C_DEV, &com), WHAL_EINVAL);

    com.freq = 100000;
    com.addrSz = 8;
    WHAL_ASSERT_EQ(whal_I2c_StartCom(BOARD_I2C_DEV, &com), WHAL_EINVAL);
}

void whal_Test_I2c_Platform(void)
{
    WHAL_TEST_SUITE_START("i2c_platform");
    WHAL_TEST(Test_Stm32wb_I2c_Timing);
    WHAL_TEST(Test_Stm32wb_I2c_Address);
    WHAL_TEST(Test_Stm32wb_I2c_Reject);
    WHAL_TEST_SUITE_END();
}
