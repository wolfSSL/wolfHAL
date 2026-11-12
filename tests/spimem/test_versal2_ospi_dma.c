/* test_versal2_ospi_dma.c
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
#include <string.h>
#include <wolfHAL/spimem/versal2_ospi_dma.h>
#include "../test.h"

/* Plain memory has no write-1-to-clear, so the Wait tests preset DST_I_STS
 * and check what the driver wrote back. */

#define SRC_RD_ADDR   0x1000
#define DST_ADDR      0x1800
#define DST_SIZE      0x1804
#define DST_CTRL      0x180C
#define DST_I_STS     0x1814
#define DST_ADDR_MSB  0x1828

#define STS_DONE           (1UL << 1)
#define STS_AXI_BRESP_ERR  (1UL << 2)
#define STS_TIMEOUT_STRM   (1UL << 3)
#define STS_TIMEOUT_MEM    (1UL << 4)
#define STS_THRESH_HIT     (1UL << 5)
#define STS_FIFO_OVERFLOW  (1UL << 7)

#define SENTINEL      0xA5A5A5A5UL

static uint32_t g_regs[(DST_ADDR_MSB / 4) + 1];
static uint32_t g_buf[16];
static whal_Versal2_OspiDma_Ctx g_ctx;

/* Cache callback records */
static int g_cleanCalls;
static int g_invalCalls;
static void *g_cbBuf;
static size_t g_cbLen;
static uint32_t g_cleanSeenSize;
static uint32_t g_cleanSeenAddr;
static uint32_t g_invalSeenSts;

static uint32_t Reg(size_t offset)
{
    return g_regs[offset / 4];
}

static void SetReg(size_t offset, uint32_t value)
{
    g_regs[offset / 4] = value;
}

static void CleanInvalCb(void *buf, size_t len)
{
    g_cleanCalls++;
    g_cbBuf = buf;
    g_cbLen = len;
    g_cleanSeenSize = Reg(DST_SIZE);
    g_cleanSeenAddr = Reg(DST_ADDR);
}

static void InvalCb(void *buf, size_t len)
{
    g_invalCalls++;
    g_cbBuf = buf;
    g_cbLen = len;
    g_invalSeenSts = Reg(DST_I_STS);
}

/* Fill every register with a sentinel and set up a context with
 * cache callbacks and no timeout. */
static void Reset(void)
{
    size_t i;

    for (i = 0; i < sizeof(g_regs) / sizeof(g_regs[0]); i++) {
        g_regs[i] = SENTINEL;
    }
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.base = (size_t)(uintptr_t)g_regs;
    g_ctx.srcRdAddr = WHAL_VERSAL2_OSPI_DMA_SRC_RD_ADDR;
    g_ctx.dstCtrl = WHAL_VERSAL2_OSPI_DMA_DST_CTRL;
    g_ctx.CacheCleanInval = CleanInvalCb;
    g_ctx.CacheInval = InvalCb;
    g_ctx.timeout = NULL;

    g_cleanCalls = 0;
    g_invalCalls = 0;
    g_cbBuf = NULL;
    g_cbLen = 0;
    g_cleanSeenSize = 0;
    g_cleanSeenAddr = 0;
    g_invalSeenSts = 0;
}

/* Arm writes the address, source, control and size registers */
static void Test_Versal2_OspiDma_ArmRegs(void)
{
    uint64_t dst = (uint64_t)(uintptr_t)g_buf;
    whal_Error err;

    Reset();
    err = whal_Versal2_OspiDma_Arm(&g_ctx, g_buf, sizeof(g_buf));
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(DST_ADDR), (uint32_t)dst);
    WHAL_ASSERT_EQ(Reg(DST_ADDR_MSB), (uint32_t)(dst >> 32));
    WHAL_ASSERT_EQ(Reg(SRC_RD_ADDR), 0xC0000000UL);
    WHAL_ASSERT_EQ(Reg(DST_CTRL), 0xF43FFA00UL);
    WHAL_ASSERT_EQ(Reg(DST_SIZE), sizeof(g_buf));
    /* Arm does not touch the status register */
    WHAL_ASSERT_EQ(Reg(DST_I_STS), SENTINEL);
}

/* Arm writes the ctx values, not fixed ones */
static void Test_Versal2_OspiDma_ArmCtxValues(void)
{
    whal_Error err;

    Reset();
    g_ctx.srcRdAddr = 0x12345678;
    g_ctx.dstCtrl = 0x803FFA00;
    err = whal_Versal2_OspiDma_Arm(&g_ctx, g_buf, 4);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(SRC_RD_ADDR), 0x12345678);
    WHAL_ASSERT_EQ(Reg(DST_CTRL), 0x803FFA00);
    WHAL_ASSERT_EQ(Reg(DST_SIZE), 4);
}

/* Arm refuses bad arguments and writes nothing */
static void Test_Versal2_OspiDma_ArmInvalid(void)
{
    uint8_t *b = (uint8_t *)g_buf;

    Reset();
    WHAL_ASSERT_EQ(whal_Versal2_OspiDma_Arm(NULL, g_buf, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Versal2_OspiDma_Arm(&g_ctx, NULL, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Versal2_OspiDma_Arm(&g_ctx, g_buf, 0), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Versal2_OspiDma_Arm(&g_ctx, b + 1, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Versal2_OspiDma_Arm(&g_ctx, b + 2, 16), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Versal2_OspiDma_Arm(&g_ctx, g_buf, 6), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Versal2_OspiDma_Arm(&g_ctx, g_buf, 17), WHAL_EINVAL);

    WHAL_ASSERT_EQ(g_cleanCalls, 0);
    WHAL_ASSERT_EQ(Reg(DST_ADDR), SENTINEL);
    WHAL_ASSERT_EQ(Reg(DST_SIZE), SENTINEL);
}

/* Arm cleans the cache before any register write, so before DST_SIZE */
static void Test_Versal2_OspiDma_ArmCacheOrder(void)
{
    whal_Error err;

    Reset();
    err = whal_Versal2_OspiDma_Arm(&g_ctx, g_buf, sizeof(g_buf));
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_cleanCalls, 1);
    WHAL_ASSERT_EQ(g_invalCalls, 0);
    WHAL_ASSERT_EQ(g_cleanSeenSize, SENTINEL);
    WHAL_ASSERT_EQ(g_cleanSeenAddr, SENTINEL);
    WHAL_ASSERT_EQ(g_cbBuf == (void *)g_buf, 1);
    WHAL_ASSERT_EQ(g_cbLen, sizeof(g_buf));
}

/* Wait on DONE: clears DONE, then invalidates the cache */
static void Test_Versal2_OspiDma_WaitDone(void)
{
    whal_Error err;

    Reset();
    SetReg(DST_I_STS, STS_DONE | STS_THRESH_HIT);
    err = whal_Versal2_OspiDma_Wait(&g_ctx, g_buf, sizeof(g_buf));
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    /* The read status was written back (W1C) */
    WHAL_ASSERT_EQ(Reg(DST_I_STS), STS_DONE | STS_THRESH_HIT);
    WHAL_ASSERT_EQ(g_invalCalls, 1);
    WHAL_ASSERT_EQ(g_cleanCalls, 0);
    WHAL_ASSERT_EQ(g_invalSeenSts, STS_DONE | STS_THRESH_HIT);
    WHAL_ASSERT_EQ(g_cbBuf == (void *)g_buf, 1);
    WHAL_ASSERT_EQ(g_cbLen, sizeof(g_buf));
}

/* Each error bit fails the wait, with or without DONE */
static void Test_Versal2_OspiDma_WaitErrors(void)
{
    static const uint32_t errs[] = {
        STS_AXI_BRESP_ERR, STS_TIMEOUT_STRM, STS_TIMEOUT_MEM,
        STS_FIFO_OVERFLOW,
    };
    size_t i;
    whal_Error err;

    for (i = 0; i < sizeof(errs) / sizeof(errs[0]); i++) {
        Reset();
        SetReg(DST_I_STS, errs[i]);
        err = whal_Versal2_OspiDma_Wait(&g_ctx, g_buf, sizeof(g_buf));
        WHAL_ASSERT_EQ(err, WHAL_EHARDWARE);
        WHAL_ASSERT_EQ(Reg(DST_I_STS), errs[i]);
        WHAL_ASSERT_EQ(g_invalCalls, 0);

        Reset();
        SetReg(DST_I_STS, errs[i] | STS_DONE);
        err = whal_Versal2_OspiDma_Wait(&g_ctx, g_buf, sizeof(g_buf));
        WHAL_ASSERT_EQ(err, WHAL_EHARDWARE);
        WHAL_ASSERT_EQ(Reg(DST_I_STS), errs[i] | STS_DONE);
        WHAL_ASSERT_EQ(g_invalCalls, 0);
    }
}

/* Wait refuses a NULL ctx */
static void Test_Versal2_OspiDma_WaitInvalid(void)
{
    Reset();
    WHAL_ASSERT_EQ(whal_Versal2_OspiDma_Wait(NULL, g_buf, 16), WHAL_EINVAL);
}

/* Arm and Wait work without cache callbacks */
static void Test_Versal2_OspiDma_NoCallbacks(void)
{
    whal_Error err;

    Reset();
    g_ctx.CacheCleanInval = NULL;
    g_ctx.CacheInval = NULL;
    err = whal_Versal2_OspiDma_Arm(&g_ctx, g_buf, sizeof(g_buf));
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(DST_SIZE), sizeof(g_buf));

    SetReg(DST_I_STS, STS_DONE);
    err = whal_Versal2_OspiDma_Wait(&g_ctx, g_buf, sizeof(g_buf));
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_cleanCalls, 0);
    WHAL_ASSERT_EQ(g_invalCalls, 0);
}

#ifndef WHAL_CFG_NO_TIMEOUT

/* Fake tick: moves forward on every read */
#ifdef WHAL_CFG_64BIT_TICK
static uint64_t g_fakeTick;

static uint64_t FakeTick(void)
{
    return g_fakeTick++;
}
#else
static uint32_t g_fakeTick;

static uint32_t FakeTick(void)
{
    return g_fakeTick++;
}
#endif

static whal_Timeout g_timeout = {
    .timeoutTicks = 10,
    .GetTick = FakeTick,
};

/* Wait times out when DONE never comes; status is not touched */
static void Test_Versal2_OspiDma_WaitTimeout(void)
{
    whal_Error err;

    Reset();
    g_fakeTick = 0;
    g_ctx.timeout = &g_timeout;
    SetReg(DST_I_STS, 0);
    err = whal_Versal2_OspiDma_Wait(&g_ctx, g_buf, sizeof(g_buf));
    WHAL_ASSERT_EQ(err, WHAL_ETIMEOUT);
    WHAL_ASSERT_EQ(Reg(DST_I_STS), 0);
    WHAL_ASSERT_EQ(g_invalCalls, 0);
}

/* With a timeout set, DONE still succeeds */
static void Test_Versal2_OspiDma_WaitDoneWithTimeout(void)
{
    whal_Error err;

    Reset();
    g_fakeTick = 0;
    g_ctx.timeout = &g_timeout;
    SetReg(DST_I_STS, STS_DONE);
    err = whal_Versal2_OspiDma_Wait(&g_ctx, g_buf, sizeof(g_buf));
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_invalCalls, 1);
}

#endif /* !WHAL_CFG_NO_TIMEOUT */

void whal_Test_Versal2_OspiDma(void)
{
    WHAL_TEST_SUITE_START("versal2_ospi_dma");
    WHAL_TEST(Test_Versal2_OspiDma_ArmRegs);
    WHAL_TEST(Test_Versal2_OspiDma_ArmCtxValues);
    WHAL_TEST(Test_Versal2_OspiDma_ArmInvalid);
    WHAL_TEST(Test_Versal2_OspiDma_ArmCacheOrder);
    WHAL_TEST(Test_Versal2_OspiDma_WaitDone);
    WHAL_TEST(Test_Versal2_OspiDma_WaitErrors);
    WHAL_TEST(Test_Versal2_OspiDma_WaitInvalid);
    WHAL_TEST(Test_Versal2_OspiDma_NoCallbacks);
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_TEST(Test_Versal2_OspiDma_WaitTimeout);
    WHAL_TEST(Test_Versal2_OspiDma_WaitDoneWithTimeout);
#endif
    WHAL_TEST_SUITE_END();
}
