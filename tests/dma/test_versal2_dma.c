/* test_versal2_dma.c
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
#include <wolfHAL/reg.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * Versal Gen 2 OSPI DMA on the real (or QEMU) registers. Real transfers
 * are tested through the OSPI reads in the spimem and flash tests.
 */

/* Register offsets from the OSPI base (AM028 OSPIDMA_*) */
#define T_DST_CTRL     0x180C
#define T_DST_I_STS    0x1814
#define T_DST_ADDR_MSB 0x1828

static uint8_t g_buf[64] __attribute__((aligned(64)));

/* The board DMA with a short timeout for the Wait test */
static whal_Timeout g_shortTimeout;
static whal_DmaMemXfer g_xfer;
static whal_Versal2_OspiDma_Cfg g_cfg;
static whal_Dma g_dma = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = &whal_Versal2_OspiDma_Driver,
    .cfg = &g_cfg,
};

static void Setup(void)
{
    const whal_Versal2_OspiDma_Cfg *board = g_whalOspiDma.cfg;

    g_shortTimeout = g_whalTimeout;
    g_shortTimeout.timeoutTicks = 10;
    g_cfg = *board;
    g_cfg.timeout = &g_shortTimeout;
    g_cfg.xfer = &g_xfer;
}

static uint32_t Reg(uint32_t off)
{
    return whal_Reg32_Read(WHAL_VERSAL2_OSPI_BASE, off);
}

static void Test_Versal2Dma_Init(void)
{
    WHAL_ASSERT_EQ(whal_Dma_Init(&g_dma), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_xfer.len, 0);
    WHAL_ASSERT_EQ(whal_Dma_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Dma_Deinit(&g_dma), WHAL_SUCCESS);
}

/* The address registers read back the DMA's progress, not the written
 * value (AM028 DST_ADDR; SRC_RD_ADDR measured on the VEK385). */
static void Test_Versal2Dma_Configure(void)
{
    whal_DmaMemXfer x = { g_buf, sizeof(g_buf) };
    whal_DmaMemXfer max = { g_buf, WHAL_VERSAL2_OSPI_DMA_MAX_LEN };
    void (*clean)(void *buf, size_t len) = g_cfg.CacheCleanInval;
    whal_Error err;

    /* The largest length is taken (set up only, so no 512 MB flush) */
    g_cfg.CacheCleanInval = NULL;
    err = whal_Dma_Configure(&g_dma, WHAL_VERSAL2_OSPI_DMA_CH, &max);
    g_cfg.CacheCleanInval = clean;
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_dma, WHAL_VERSAL2_OSPI_DMA_CH, &x),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(T_DST_ADDR_MSB), 0);
    WHAL_ASSERT_EQ(Reg(T_DST_CTRL), WHAL_VERSAL2_OSPI_DMA_DST_CTRL);
    WHAL_ASSERT_EQ(g_xfer.buf == g_buf, 1);
    WHAL_ASSERT_EQ(g_xfer.len, sizeof(g_buf));
}

static void Test_Versal2Dma_BadArgs(void)
{
    whal_DmaMemXfer x = { g_buf, sizeof(g_buf) };
    whal_DmaMemXfer noBuf = { NULL, 4 };
    whal_DmaMemXfer noLen = { g_buf, 0 };
    whal_DmaMemXfer oddBuf = { &g_buf[1], 4 };
    whal_DmaMemXfer oddLen = { g_buf, 6 };
    whal_DmaMemXfer bigLen = { g_buf, WHAL_VERSAL2_OSPI_DMA_MAX_LEN + 4 };
    size_t ch = WHAL_VERSAL2_OSPI_DMA_CH;

    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_dma, ch, NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_dma, ch, &noBuf), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_dma, ch, &noLen), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_dma, ch, &oddBuf), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_dma, ch, &oddLen), WHAL_EINVAL);
    /* More than the DST_SIZE field holds (AM028: SIZE is bits 28:2) */
    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_dma, ch, &bigLen), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_dma, ch + 1, &x), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Dma_Start(&g_dma, ch + 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Dma_Stop(&g_dma, ch + 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Dma_Wait(&g_dma, ch + 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Dma_Deinit(NULL), WHAL_EINVAL);
}

/* Init forgets the last transfer, so Start has nothing to start. */
static void Test_Versal2Dma_StartNotConfigured(void)
{
    WHAL_ASSERT_EQ(whal_Dma_Init(&g_dma), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Dma_Start(&g_dma, WHAL_VERSAL2_OSPI_DMA_CH),
                   WHAL_EINVAL);
}

/* Nothing started: no DONE ever comes, so Wait times out. */
static void Test_Versal2Dma_WaitTimeout(void)
{
#ifdef WHAL_CFG_NO_TIMEOUT
    WHAL_SKIP();
#else
    WHAL_ASSERT_EQ(whal_Dma_Init(&g_dma), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Dma_Wait(&g_dma, WHAL_VERSAL2_OSPI_DMA_CH),
                   WHAL_ETIMEOUT);
#endif
}

static void Test_Versal2Dma_StopClearsStatus(void)
{
    WHAL_ASSERT_EQ(whal_Dma_Stop(&g_dma, WHAL_VERSAL2_OSPI_DMA_CH),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(T_DST_I_STS), 0);
}

/* MapSelect sets and clears OSPI_MUX_SEL and leaves the other bits. */
static void Test_Versal2Dma_MapSelect(void)
{
    uint32_t saved = whal_Reg32_Read(WHAL_VERSAL2_OSPI_AXI_SEL_ADDR, 0);
    uint32_t other = saved & ~WHAL_VERSAL2_OSPI_MUX_SEL_Msk;

    whal_Versal2_Ospi_MapSelect(1);
    WHAL_ASSERT_EQ(whal_Reg32_Read(WHAL_VERSAL2_OSPI_AXI_SEL_ADDR, 0),
                   other | WHAL_VERSAL2_OSPI_MUX_SEL_Msk);
    whal_Versal2_Ospi_MapSelect(0);
    WHAL_ASSERT_EQ(whal_Reg32_Read(WHAL_VERSAL2_OSPI_AXI_SEL_ADDR, 0),
                   other);
    whal_Reg32_Write(WHAL_VERSAL2_OSPI_AXI_SEL_ADDR, 0, saved);
}

void whal_Test_Dma_Platform(void)
{
    Setup();

    WHAL_TEST_SUITE_START("dma_platform");
    WHAL_TEST(Test_Versal2Dma_Init);
    WHAL_TEST(Test_Versal2Dma_Configure);
    WHAL_TEST(Test_Versal2Dma_BadArgs);
    WHAL_TEST(Test_Versal2Dma_StartNotConfigured);
    WHAL_TEST(Test_Versal2Dma_WaitTimeout);
    WHAL_TEST(Test_Versal2Dma_StopClearsStatus);
    WHAL_TEST(Test_Versal2Dma_MapSelect);
    WHAL_TEST_SUITE_END();

    /* Leave the board DMA as Board_Init set it up. */
    (void)whal_Dma_Init(&g_whalOspiDma);
}
