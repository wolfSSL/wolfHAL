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

/* Each helper must touch only its own register. Wide values are compared
 * with == because WHAL_ASSERT_EQ cuts them to int. */

#define SENTINEL 0xA5A5A5A5UL

static uint32_t g_regs[3];

static uintptr_t RegBase(void)
{
    return (uintptr_t)g_regs;
}

static void ResetRegs(void)
{
    g_regs[0] = SENTINEL;
    g_regs[1] = 0;
    g_regs[2] = SENTINEL;
}

#ifdef WHAL_HAVE_REG64
#define SENTINEL64 0xA5A5A5A5A5A5A5A5ULL
#define REG64_BIT  ((uint64_t)1 << 40)

static uint64_t g_regs64[3];

static uintptr_t Reg64Base(void)
{
    return (uintptr_t)g_regs64;
}

static void ResetRegs64(void)
{
    g_regs64[0] = SENTINEL64;
    g_regs64[1] = 0;
    g_regs64[2] = SENTINEL64;
}
#endif /* WHAL_HAVE_REG64 */

#ifndef WHAL_CFG_NO_TIMEOUT

#ifdef WHAL_CFG_64BIT_TICK
typedef uint64_t RegTick_t;
#else
typedef uint32_t RegTick_t;
#endif

static RegTick_t g_regTick;
static RegTick_t g_regSetAt;
static void (*g_regSetFn)(void);
static whal_Timeout g_regTimeout;

/* Advances on each read so a poll can time out. Calls g_regSetFn at tick
 * g_regSetAt to set the polled bit. */
static RegTick_t RegTick(void)
{
    if (g_regSetFn != NULL && g_regTick == g_regSetAt) {
        g_regSetFn();
    }
    return g_regTick++;
}

static whal_Timeout *RegTimeout(RegTick_t setAt, void (*setFn)(void))
{
    g_regTick = 0;
    g_regSetAt = setAt;
    g_regSetFn = setFn;
    g_regTimeout.timeoutTicks = 10;
    g_regTimeout.GetTick = RegTick;
    return &g_regTimeout;
}

static void SetBit32(void)
{
    g_regs[1] |= 0x80;
}

#ifdef WHAL_HAVE_REG64
static void SetBit64(void)
{
    g_regs64[1] |= REG64_BIT;
}
#endif

#endif /* !WHAL_CFG_NO_TIMEOUT */

/* A bounded wait, so a broken poll fails instead of hanging. NULL when
 * timeouts are compiled out. */
static whal_Timeout *Bounded(void)
{
#ifdef WHAL_CFG_NO_TIMEOUT
    return NULL;
#else
    return RegTimeout(0, NULL);
#endif
}

/* whal_Reg32_* */

static void Test_Reg32_WriteOnlyItsWord(void)
{
    ResetRegs();
    whal_Reg32_Write(RegBase(), 4, 0x12345678);
    WHAL_ASSERT_EQ(g_regs[1], 0x12345678);
    WHAL_ASSERT_EQ(g_regs[0], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[2], SENTINEL);
}

static void Test_Reg32_ReadOnlyItsWord(void)
{
    ResetRegs();
    g_regs[1] = 0x8000BEEF;
    WHAL_ASSERT_EQ(whal_Reg32_Read(RegBase(), 4) == 0x8000BEEFUL, 1);
}

static void Test_Reg32_UpdateOnlyItsField(void)
{
    ResetRegs();
    /* Field bits start set, so the update must clear them */
    g_regs[1] = 0xFFFF00F0;
    whal_Reg32_Update(RegBase(), 4, 0x000000F0, 0x00000050);
    WHAL_ASSERT_EQ(g_regs[1], 0xFFFF0050);
    WHAL_ASSERT_EQ(g_regs[0], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[2], SENTINEL);
}

static void Test_Reg32_GetField(void)
{
    uint32_t val = 0;

    ResetRegs();
    g_regs[1] = 0xFFFFFFFF;
    whal_Reg32_Get(RegBase(), 4, 0x00003C00, 10, &val);
    WHAL_ASSERT_EQ(val, 0xF);
    whal_Reg32_Get(RegBase(), 4, 0x80000000, 31, &val);
    WHAL_ASSERT_EQ(val, 1);
}

static void Test_Reg32_ReadPoll(void)
{
    ResetRegs();
    /* Bits outside the mask are set and must be ignored */
    g_regs[1] = 0xFF;
    WHAL_ASSERT_EQ(whal_Reg32_ReadPoll(RegBase(), 4, 0x80, 0x80, Bounded()),
                   WHAL_SUCCESS);
}

/* Value 0 waits for the masked bits to clear */
static void Test_Reg32_ReadPollClear(void)
{
    ResetRegs();
    g_regs[1] = 0x7F;
    WHAL_ASSERT_EQ(whal_Reg32_ReadPoll(RegBase(), 4, 0x80, 0, Bounded()),
                   WHAL_SUCCESS);
}

#ifndef WHAL_CFG_NO_TIMEOUT

static void Test_Reg32_ReadPollRetry(void)
{
    ResetRegs();
    /* Bit 0x80 sets at tick 3; the other bits are already set */
    g_regs[1] = 0x7F;
    WHAL_ASSERT_EQ(whal_Reg32_ReadPoll(RegBase(), 4, 0x80, 0x80,
                                       RegTimeout(3, SetBit32)),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regTick > 3, 1);
}

static void Test_Reg32_ReadPollTimeout(void)
{
    ResetRegs();
    WHAL_ASSERT_EQ(whal_Reg32_ReadPoll(RegBase(), 4, 0x80, 0x80,
                                       RegTimeout(0, NULL)),
                   WHAL_ETIMEOUT);
}

static void Test_Reg32_ReadPollClearTimeout(void)
{
    ResetRegs();
    g_regs[1] = 0x80;
    WHAL_ASSERT_EQ(whal_Reg32_ReadPoll(RegBase(), 4, 0x80, 0,
                                       RegTimeout(0, NULL)),
                   WHAL_ETIMEOUT);
}

#endif /* !WHAL_CFG_NO_TIMEOUT */

/* whal_Reg64_* */

#ifdef WHAL_HAVE_REG64

static void Test_Reg64_WriteOnlyItsWord(void)
{
    ResetRegs64();
    whal_Reg64_Write(Reg64Base(), 8, 0x0123456789ABCDEFULL);
    WHAL_ASSERT_EQ(g_regs64[1] == 0x0123456789ABCDEFULL, 1);
    WHAL_ASSERT_EQ(g_regs64[0] == SENTINEL64, 1);
    WHAL_ASSERT_EQ(g_regs64[2] == SENTINEL64, 1);
}

static void Test_Reg64_ReadOnlyItsWord(void)
{
    ResetRegs64();
    g_regs64[1] = 0xFEDCBA9876543210ULL;
    WHAL_ASSERT_EQ(whal_Reg64_Read(Reg64Base(), 8) == 0xFEDCBA9876543210ULL,
                   1);
}

static void Test_Reg64_UpdateOnlyItsField(void)
{
    ResetRegs64();
    /* Both halves of the field start set, so the update must clear them */
    g_regs64[1] = 0xFFFFFFFFFFFF00F0ULL;
    whal_Reg64_Update(Reg64Base(), 8, 0x0000FF00000000F0ULL,
                      0x0000AB0000000050ULL);
    WHAL_ASSERT_EQ(g_regs64[1] == 0xFFFFABFFFFFF0050ULL, 1);
    WHAL_ASSERT_EQ(g_regs64[0] == SENTINEL64, 1);
    WHAL_ASSERT_EQ(g_regs64[2] == SENTINEL64, 1);
}

static void Test_Reg64_GetField(void)
{
    uint64_t val = 0;

    ResetRegs64();
    g_regs64[1] = 0xFFFFFFFFFFFFFFFFULL;
    whal_Reg64_Get(Reg64Base(), 8, 0x00003C0000000000ULL, 42, &val);
    WHAL_ASSERT_EQ(val == 0xF, 1);
    whal_Reg64_Get(Reg64Base(), 8, 0x0000003C00000000ULL, 34, &val);
    WHAL_ASSERT_EQ(val == 0xF, 1);
}

static void Test_Reg64_ReadPoll(void)
{
    ResetRegs64();
    /* Bits outside the mask are set and must be ignored */
    g_regs64[1] = REG64_BIT | 0xFFFFFFFFULL;
    WHAL_ASSERT_EQ(whal_Reg64_ReadPoll(Reg64Base(), 8, REG64_BIT, REG64_BIT,
                                       Bounded()),
                   WHAL_SUCCESS);
}

static void Test_Reg64_ReadPollClear(void)
{
    ResetRegs64();
    g_regs64[1] = ~REG64_BIT;
    WHAL_ASSERT_EQ(whal_Reg64_ReadPoll(Reg64Base(), 8, REG64_BIT, 0,
                                       Bounded()),
                   WHAL_SUCCESS);
}

#ifndef WHAL_CFG_NO_TIMEOUT

static void Test_Reg64_ReadPollRetry(void)
{
    ResetRegs64();
    /* Bit 40 sets at tick 3; the low word is already set */
    g_regs64[1] = 0xFFFFFFFFULL;
    WHAL_ASSERT_EQ(whal_Reg64_ReadPoll(Reg64Base(), 8, REG64_BIT, REG64_BIT,
                                       RegTimeout(3, SetBit64)),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regTick > 3, 1);
}

static void Test_Reg64_ReadPollTimeout(void)
{
    ResetRegs64();
    WHAL_ASSERT_EQ(whal_Reg64_ReadPoll(Reg64Base(), 8, REG64_BIT, REG64_BIT,
                                       RegTimeout(0, NULL)),
                   WHAL_ETIMEOUT);
}

static void Test_Reg64_ReadPollClearTimeout(void)
{
    ResetRegs64();
    g_regs64[1] = REG64_BIT;
    WHAL_ASSERT_EQ(whal_Reg64_ReadPoll(Reg64Base(), 8, REG64_BIT, 0,
                                       RegTimeout(0, NULL)),
                   WHAL_ETIMEOUT);
}

#endif /* !WHAL_CFG_NO_TIMEOUT */

#endif /* WHAL_HAVE_REG64 */

/* Legacy whal_Reg_* (WHAL_CFG_REG_LEGACY) */

#ifdef WHAL_CFG_REG_LEGACY

static size_t LegacyBase(void)
{
    return (size_t)(uintptr_t)g_regs;
}

static void Test_RegLegacy_WriteOnlyItsWord(void)
{
    ResetRegs();
    WHAL_ASSERT_EQ(whal_Reg_Write(LegacyBase(), 4, 0x12345678),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[1], 0x12345678);
    WHAL_ASSERT_EQ(g_regs[0], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[2], SENTINEL);
    /* The largest 32-bit value is accepted */
    WHAL_ASSERT_EQ(whal_Reg_Write(LegacyBase(), 4, 0xFFFFFFFFUL),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[1] == 0xFFFFFFFFUL, 1);
    WHAL_ASSERT_EQ(g_regs[2], SENTINEL);
}

static void Test_RegLegacy_ReadOnlyItsWord(void)
{
    ResetRegs();
    g_regs[1] = 0x8000BEEF;
    WHAL_ASSERT_EQ(whal_Reg_Read(LegacyBase(), 4) == 0x8000BEEFUL, 1);
}

static void Test_RegLegacy_UpdateOnlyItsField(void)
{
    ResetRegs();
    g_regs[1] = 0xFFFF00F0;
    WHAL_ASSERT_EQ(whal_Reg_Update(LegacyBase(), 4, 0x000000F0, 0x00000050),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[1], 0xFFFF0050);
    WHAL_ASSERT_EQ(g_regs[0], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[2], SENTINEL);
}

static void Test_RegLegacy_GetField(void)
{
    size_t val = 0;

    ResetRegs();
    g_regs[1] = 0xFFFFFFFF;
    WHAL_ASSERT_EQ(whal_Reg_Get(LegacyBase(), 4, 0x00003C00, 10, &val),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val, 0xF);
    WHAL_ASSERT_EQ(whal_Reg_Get(LegacyBase(), 4, 0x80000000UL, 31, &val),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val, 1);
}

static void Test_RegLegacy_ReadPoll(void)
{
    ResetRegs();
    g_regs[1] = 0x7F;
    WHAL_ASSERT_EQ(whal_Reg_ReadPoll(LegacyBase(), 4, 0x80, 0, Bounded()),
                   WHAL_SUCCESS);
    g_regs[1] = 0xFF;
    WHAL_ASSERT_EQ(whal_Reg_ReadPoll(LegacyBase(), 4, 0x80, 0x80, Bounded()),
                   WHAL_SUCCESS);
}

#ifndef WHAL_CFG_NO_TIMEOUT
static void Test_RegLegacy_ReadPollTimeout(void)
{
    ResetRegs();
    WHAL_ASSERT_EQ(whal_Reg_ReadPoll(LegacyBase(), 4, 0x80, 0x80,
                                     RegTimeout(0, NULL)),
                   WHAL_ETIMEOUT);
}
#endif

#if SIZE_MAX > 0xFFFFFFFFu
/* Low 32 bits are valid, so a missing check would write or match */
#define LEGACY_WIDE (((size_t)1 << 32) | 0x80)

static void Test_RegLegacy_WideRejected(void)
{
    size_t val = 123;

    ResetRegs();
    g_regs[1] = 0x11111111;
    WHAL_ASSERT_EQ(whal_Reg_Write(LegacyBase(), 4, LEGACY_WIDE), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Reg_Update(LegacyBase(), 4, LEGACY_WIDE, 0x80),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Reg_Update(LegacyBase(), 4, 0xFF, LEGACY_WIDE),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(g_regs[1], 0x11111111);
    WHAL_ASSERT_EQ(g_regs[0], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[2], SENTINEL);

    WHAL_ASSERT_EQ(whal_Reg_Get(LegacyBase(), 4, LEGACY_WIDE, 0, &val),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(val == 0, 1);

    g_regs[1] = 0x80;
    WHAL_ASSERT_EQ(whal_Reg_ReadPoll(LegacyBase(), 4, LEGACY_WIDE, 0x80,
                                     Bounded()),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Reg_ReadPoll(LegacyBase(), 4, 0x80, LEGACY_WIDE,
                                     Bounded()),
                   WHAL_EINVAL);
}
#endif /* SIZE_MAX > 0xFFFFFFFFu */

#endif /* WHAL_CFG_REG_LEGACY */

void whal_Test_Reg(void)
{
    WHAL_TEST_SUITE_START("reg");
    WHAL_TEST(Test_Reg32_WriteOnlyItsWord);
    WHAL_TEST(Test_Reg32_ReadOnlyItsWord);
    WHAL_TEST(Test_Reg32_UpdateOnlyItsField);
    WHAL_TEST(Test_Reg32_GetField);
    WHAL_TEST(Test_Reg32_ReadPoll);
    WHAL_TEST(Test_Reg32_ReadPollClear);
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_TEST(Test_Reg32_ReadPollRetry);
    WHAL_TEST(Test_Reg32_ReadPollTimeout);
    WHAL_TEST(Test_Reg32_ReadPollClearTimeout);
#endif

#ifdef WHAL_HAVE_REG64
    WHAL_TEST(Test_Reg64_WriteOnlyItsWord);
    WHAL_TEST(Test_Reg64_ReadOnlyItsWord);
    WHAL_TEST(Test_Reg64_UpdateOnlyItsField);
    WHAL_TEST(Test_Reg64_GetField);
    WHAL_TEST(Test_Reg64_ReadPoll);
    WHAL_TEST(Test_Reg64_ReadPollClear);
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_TEST(Test_Reg64_ReadPollRetry);
    WHAL_TEST(Test_Reg64_ReadPollTimeout);
    WHAL_TEST(Test_Reg64_ReadPollClearTimeout);
#endif
#endif

#ifdef WHAL_CFG_REG_LEGACY
    WHAL_TEST(Test_RegLegacy_WriteOnlyItsWord);
    WHAL_TEST(Test_RegLegacy_ReadOnlyItsWord);
    WHAL_TEST(Test_RegLegacy_UpdateOnlyItsField);
    WHAL_TEST(Test_RegLegacy_GetField);
    WHAL_TEST(Test_RegLegacy_ReadPoll);
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_TEST(Test_RegLegacy_ReadPollTimeout);
#endif
#if SIZE_MAX > 0xFFFFFFFFu
    WHAL_TEST(Test_RegLegacy_WideRejected);
#endif
#endif
    WHAL_TEST_SUITE_END();
}
