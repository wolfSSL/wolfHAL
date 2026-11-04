/* test_reg.c
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


#include <stdint.h>
#include <wolfHAL/wolfHAL.h>
#include "../test.h"

/* Each helper must touch only its own 32-bit word, also where size_t is
 * 64 bits. */

#define SENTINEL 0xA5A5A5A5UL

static uint32_t g_regs[3];

static size_t RegBase(void)
{
    return (size_t)(uintptr_t)g_regs;
}

static void ResetRegs(void)
{
    g_regs[0] = SENTINEL;
    g_regs[1] = 0;
    g_regs[2] = SENTINEL;
}

static void Test_Reg_WriteOnlyItsWord(void)
{
    ResetRegs();
    whal_Reg_Write(RegBase(), 4, 0x12345678);
    WHAL_ASSERT_EQ(g_regs[1], 0x12345678);
    WHAL_ASSERT_EQ(g_regs[0], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[2], SENTINEL);
}

static void Test_Reg_ReadOnlyItsWord(void)
{
    ResetRegs();
    g_regs[1] = 0x0000BEEF;
    /* Compare the full size_t; WHAL_ASSERT_EQ would cut it to int */
    WHAL_ASSERT_EQ(whal_Reg_Read(RegBase(), 4) == 0x0000BEEF, 1);
}

static void Test_Reg_UpdateOnlyItsField(void)
{
    ResetRegs();
    /* Field bits start set, so the update must clear them */
    g_regs[1] = 0xFFFF00F0;
    whal_Reg_Update(RegBase(), 4, 0x000000F0, 0x00000050);
    WHAL_ASSERT_EQ(g_regs[1], 0xFFFF0050);
    WHAL_ASSERT_EQ(g_regs[0], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[2], SENTINEL);
}

static void Test_Reg_GetField(void)
{
    size_t val = 0;

    ResetRegs();
    g_regs[1] = 0x00003C00;
    whal_Reg_Get(RegBase(), 4, 0x00003C00, 10, &val);
    WHAL_ASSERT_EQ(val, 0xF);
}

/* Upper and lower halves differ, so keeping the wrong half fails. The
 * double shift is 0 where size_t is 32 bits. */
#define REG_WIDE_VALUE (((size_t)0xA5A5A5A5 << 16 << 16) | 0x12345678)

/* Bits above 31 are dropped. Compare full size_t values; WHAL_ASSERT_EQ
 * cuts them to int. */
static void Test_Reg_HighBitsDropped(void)
{
    size_t val = 0;

    ResetRegs();
    whal_Reg_Write(RegBase(), 4, REG_WIDE_VALUE);
    WHAL_ASSERT_EQ(g_regs[1] == 0x12345678UL, 1);
    WHAL_ASSERT_EQ(g_regs[2], SENTINEL);
    WHAL_ASSERT_EQ(whal_Reg_Read(RegBase(), 4) == 0x12345678UL, 1);
    g_regs[1] = 0xFFFFFFFF;
    whal_Reg_Get(RegBase(), 4, (size_t)-1, 0, &val);
    WHAL_ASSERT_EQ(val == 0xFFFFFFFFUL, 1);

    g_regs[1] = 0;
    whal_Reg_Update(RegBase(), 4, (size_t)-1, REG_WIDE_VALUE);
    WHAL_ASSERT_EQ(g_regs[1] == 0x12345678UL, 1);
    WHAL_ASSERT_EQ(g_regs[0], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[2], SENTINEL);
}

#ifndef WHAL_CFG_NO_TIMEOUT

#ifdef WHAL_CFG_64BIT_TICK
typedef uint64_t RegTick_t;
#else
typedef uint32_t RegTick_t;
#endif

static RegTick_t g_regTick;
static RegTick_t g_regSetAt;
static whal_Timeout g_regTimeout;

/* Advances on each read so a poll can time out. Sets bit 0x80 at tick
 * g_regSetAt when that is not 0. */
static RegTick_t RegTick(void)
{
    if (g_regSetAt != 0 && g_regTick == g_regSetAt) {
        g_regs[1] |= 0x80;
    }
    return g_regTick++;
}

static whal_Timeout *RegTimeout(RegTick_t setAt)
{
    g_regTick = 0;
    g_regSetAt = setAt;
    g_regTimeout.timeoutTicks = 10;
    g_regTimeout.GetTick = RegTick;
    return &g_regTimeout;
}

#endif /* !WHAL_CFG_NO_TIMEOUT */

static void Test_Reg_ReadPoll(void)
{
    whal_Timeout *timeout = NULL;

    ResetRegs();
    /* Bits outside the mask are set and must be ignored */
    g_regs[1] = 0xFF;
#ifndef WHAL_CFG_NO_TIMEOUT
    /* A bounded wait, so a broken poll fails instead of hanging */
    timeout = RegTimeout(0);
#endif
    WHAL_ASSERT_EQ(whal_Reg_ReadPoll(RegBase(), 4, 0x80, 0x80, timeout),
                   WHAL_SUCCESS);
}

/* Value 0 waits for the masked bits to clear */
static void Test_Reg_ReadPollClear(void)
{
    whal_Timeout *timeout = NULL;

    ResetRegs();
    g_regs[1] = 0x7F;
#ifndef WHAL_CFG_NO_TIMEOUT
    timeout = RegTimeout(0);
#endif
    WHAL_ASSERT_EQ(whal_Reg_ReadPoll(RegBase(), 4, 0x80, 0, timeout),
                   WHAL_SUCCESS);
}

#ifndef WHAL_CFG_NO_TIMEOUT

static void Test_Reg_ReadPollRetry(void)
{
    ResetRegs();
    /* Bit 0x80 sets at tick 3; the other bits are already set */
    g_regs[1] = 0x7F;
    WHAL_ASSERT_EQ(whal_Reg_ReadPoll(RegBase(), 4, 0x80, 0x80,
                                     RegTimeout(3)),
                   WHAL_SUCCESS);
    /* The poll read the register again until the bit was set */
    WHAL_ASSERT_EQ(g_regTick > 3, 1);
}

static void Test_Reg_ReadPollTimeout(void)
{
    ResetRegs();
    WHAL_ASSERT_EQ(whal_Reg_ReadPoll(RegBase(), 4, 0x80, 0x80,
                                     RegTimeout(0)),
                   WHAL_ETIMEOUT);
}

/* A set bit never clears, so the wait for clear times out */
static void Test_Reg_ReadPollClearTimeout(void)
{
    ResetRegs();
    g_regs[1] = 0x80;
    WHAL_ASSERT_EQ(whal_Reg_ReadPoll(RegBase(), 4, 0x80, 0, RegTimeout(0)),
                   WHAL_ETIMEOUT);
}

#endif /* !WHAL_CFG_NO_TIMEOUT */

void whal_Test_Reg(void)
{
    WHAL_TEST_SUITE_START("reg");
    WHAL_TEST(Test_Reg_WriteOnlyItsWord);
    WHAL_TEST(Test_Reg_ReadOnlyItsWord);
    WHAL_TEST(Test_Reg_UpdateOnlyItsField);
    WHAL_TEST(Test_Reg_GetField);
    WHAL_TEST(Test_Reg_HighBitsDropped);
    WHAL_TEST(Test_Reg_ReadPoll);
    WHAL_TEST(Test_Reg_ReadPollClear);
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_TEST(Test_Reg_ReadPollRetry);
    WHAL_TEST(Test_Reg_ReadPollTimeout);
    WHAL_TEST(Test_Reg_ReadPollClearTimeout);
#endif
    WHAL_TEST_SUITE_END();
}
