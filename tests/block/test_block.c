/* test_block.c
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
#include <wolfHAL/block/block.h>
#include "wolfHAL_board.h"
#include "test.h"
#include "wolfHAL_peripheral.h"

static whal_Block *g_testBlockDev;
static uint8_t *g_testBuf;
static size_t g_testBlockSz;
static uint8_t g_testErasedByte;

static void Test_Block_EraseBlank(void)
{
    size_t i;

    WHAL_ASSERT_EQ(whal_Block_Erase(g_testBlockDev, 0, 1), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Block_Read(g_testBlockDev, 0,
                                    g_testBuf, 1), WHAL_SUCCESS);

    for (i = 0; i < g_testBlockSz; i++) {
        WHAL_ASSERT_EQ(g_testBuf[i], g_testErasedByte);
    }
}

static void Test_Block_WriteRead(void)
{
    uint8_t pattern[] = "wolfHAL";
    size_t i;

    for (i = 0; i < g_testBlockSz; i++)
        g_testBuf[i] = pattern[i % sizeof(pattern)];

    WHAL_ASSERT_EQ(whal_Block_Erase(g_testBlockDev, 0, 1), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Block_Write(g_testBlockDev, 0,
                                     g_testBuf, 1), WHAL_SUCCESS);

    for (i = 0; i < g_testBlockSz; i++)
        g_testBuf[i] = 0;

    WHAL_ASSERT_EQ(whal_Block_Read(g_testBlockDev, 0,
                                    g_testBuf, 1), WHAL_SUCCESS);

    for (i = 0; i < g_testBlockSz; i++) {
        WHAL_ASSERT_EQ(g_testBuf[i], pattern[i % sizeof(pattern)]);
    }
}

static void Test_Block_MultiWriteRead(void)
{
    uint8_t patternA[] = "wolfHAL_A";
    uint8_t patternB[] = "wolfHAL_B";
    size_t i;

    /* Fill block 0 with pattern A, block 1 with pattern B */
    for (i = 0; i < g_testBlockSz; i++)
        g_testBuf[i] = patternA[i % sizeof(patternA)];
    for (i = 0; i < g_testBlockSz; i++)
        g_testBuf[g_testBlockSz + i] = patternB[i % sizeof(patternB)];

    WHAL_ASSERT_EQ(whal_Block_Erase(g_testBlockDev, 0, 2), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Block_Write(g_testBlockDev, 0,
                                     g_testBuf, 2), WHAL_SUCCESS);

    for (i = 0; i < g_testBlockSz * 2; i++)
        g_testBuf[i] = 0;

    WHAL_ASSERT_EQ(whal_Block_Read(g_testBlockDev, 0,
                                    g_testBuf, 2), WHAL_SUCCESS);

    for (i = 0; i < g_testBlockSz; i++) {
        WHAL_ASSERT_EQ(g_testBuf[i],
                       patternA[i % sizeof(patternA)]);
    }
    for (i = 0; i < g_testBlockSz; i++) {
        WHAL_ASSERT_EQ(g_testBuf[g_testBlockSz + i],
                       patternB[i % sizeof(patternB)]);
    }
}

static void Test_Block_Api(void)
{
    WHAL_ASSERT_EQ(whal_Block_Read(g_testBlockDev, 0, NULL, 1), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Block_Write(g_testBlockDev, 0, NULL, 1), WHAL_EINVAL);
}

static void run_block_tests(const char *name)
{
    WHAL_TEST_SUITE_START("block");
    if (name)
        whal_Test_Printf("  device: %s\n", name);
    WHAL_TEST(Test_Block_Api);
    WHAL_TEST(Test_Block_EraseBlank);
    WHAL_TEST(Test_Block_WriteRead);
    WHAL_TEST(Test_Block_MultiWriteRead);
    WHAL_TEST_SUITE_END();
}

void whal_Test_Block(void)
{
    /* Test peripheral block devices */
    for (size_t i = 0; g_peripheralBlock[i].dev; i++) {
        g_testBlockDev = g_peripheralBlock[i].dev;
        g_testBuf = g_peripheralBlock[i].blockBuf;
        g_testBlockSz = g_peripheralBlock[i].blockSz;
        g_testErasedByte = g_peripheralBlock[i].erasedByte;
        run_block_tests(g_peripheralBlock[i].name);
    }
}
