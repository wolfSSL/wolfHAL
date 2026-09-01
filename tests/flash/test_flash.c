/* test_flash.c
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
#include "wolfHAL_board.h"
#include "test.h"
#include "wolfHAL_peripheral.h"

static whal_Flash *g_testFlashDev;
static size_t g_testFlashAddr;
static size_t g_testFlashSectorSz;

static void Test_Flash_Api(void)
{
    WHAL_ASSERT_EQ(whal_Flash_Read(g_testFlashDev, 0, NULL, 8), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Write(g_testFlashDev, 0, NULL, 8), WHAL_EINVAL);
}

static void Test_Flash_WriteReadErase(void)
{
    uint8_t pattern[32] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
    };
    uint8_t readback[32];
    whal_Error err;

    WHAL_ASSERT_EQ(whal_Flash_Unlock(g_testFlashDev, g_testFlashAddr,
                                      g_testFlashSectorSz), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, g_testFlashAddr,
                                     g_testFlashSectorSz), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Flash_Read(g_testFlashDev, g_testFlashAddr,
                                    readback, sizeof(readback)), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_NEQ(readback, pattern, sizeof(pattern));


    do {
        err = whal_Flash_Write(g_testFlashDev, g_testFlashAddr, pattern,
                               sizeof(pattern));
    } while (err == WHAL_ENOTREADY);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Flash_Read(g_testFlashDev, g_testFlashAddr,
                                    readback, sizeof(readback)), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(pattern, readback, sizeof(pattern));

    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, g_testFlashAddr,
                                     g_testFlashSectorSz), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Flash_Read(g_testFlashDev, g_testFlashAddr,
                                    readback, sizeof(readback)), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_NEQ(readback, pattern, sizeof(pattern));

    WHAL_ASSERT_EQ(whal_Flash_Lock(g_testFlashDev, g_testFlashAddr,
                                    g_testFlashSectorSz), WHAL_SUCCESS);
}

static void run_flash_tests(const char *name)
{
    WHAL_TEST_SUITE_START("flash");
    if (name)
        whal_Test_Printf("  device: %s\n", name);
    WHAL_TEST(Test_Flash_Api);
    WHAL_TEST(Test_Flash_WriteReadErase);
    WHAL_TEST_SUITE_END();
}

void whal_Test_Flash(void)
{
    /* Test on-chip flash */
    g_testFlashDev = BOARD_FLASH_DEV;
    g_testFlashAddr = BOARD_FLASH_TEST_ADDR;
    g_testFlashSectorSz = BOARD_FLASH_SECTOR_SZ;
    run_flash_tests("on-chip");

    /* Test peripheral flash devices */
    for (size_t i = 0; g_peripheralFlash[i].dev; i++) {
        g_testFlashDev = g_peripheralFlash[i].dev;
        g_testFlashAddr = 0;
        g_testFlashSectorSz = g_peripheralFlash[i].sectorSz;
        run_flash_tests(g_peripheralFlash[i].name);
    }
}
