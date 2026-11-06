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
static size_t g_testFlashStart;
static size_t g_testFlashAddr;
static size_t g_testFlashSectorSz;
static size_t g_testFlashPageSz;
static int g_testFlashOnChip;

/* Linker symbols: the .data load image follows the code in flash */
extern uint8_t _sidata[];
extern uint8_t _sdata[];
extern uint8_t _edata[];

static const uint8_t g_extentPattern[32] = {
    0x5A, 0xA5, 0x3C, 0xC3, 0x69, 0x96, 0x0F, 0xF0,
    0x5A, 0xA5, 0x3C, 0xC3, 0x69, 0x96, 0x0F, 0xF0,
    0x5A, 0xA5, 0x3C, 0xC3, 0x69, 0x96, 0x0F, 0xF0,
    0x5A, 0xA5, 0x3C, 0xC3, 0x69, 0x96, 0x0F, 0xF0,
};

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

static whal_Error WriteExtentPattern(size_t addr)
{
    whal_Error err;

    do {
        err = whal_Flash_Write(g_testFlashDev, addr, g_extentPattern,
                               sizeof(g_extentPattern));
    } while (err == WHAL_ENOTREADY);

    return err;
}

static int HasExtentPattern(size_t addr)
{
    uint8_t readback[sizeof(g_extentPattern)];
    size_t i;

    if (whal_Flash_Read(g_testFlashDev, addr, readback,
                        sizeof(readback)) != WHAL_SUCCESS)
        return -1;

    for (i = 0; i < sizeof(readback); i++) {
        if (readback[i] != g_extentPattern[i])
            return 0;
    }
    return 1;
}

/* Three sectors ending at the test sector, or starting at it if the test
 * sector is too close to the start of the device */
static size_t ExtentBase(void)
{
    if (g_testFlashAddr - g_testFlashStart >= 2 * g_testFlashSectorSz)
        return g_testFlashAddr - 2 * g_testFlashSectorSz;
    return g_testFlashAddr;
}

/* On-chip flash below the end of the running image must not be erased */
static int ExtentOverlapsImage(uintptr_t base)
{
    size_t imageEnd = (size_t)_sidata + (size_t)(_edata - _sdata);

    return g_testFlashOnChip && base < imageEnd;
}

static void Test_Flash_EraseExtent(void)
{
    const size_t sz = g_testFlashSectorSz;
    const uintptr_t base = ExtentBase();

    if (ExtentOverlapsImage(base))
        WHAL_SKIP();

    WHAL_ASSERT_EQ(whal_Flash_Unlock(g_testFlashDev, base, 3 * sz),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, base, 3 * sz),
                   WHAL_SUCCESS);

    /* Two whole sectors: both erased, the third untouched */
    WHAL_ASSERT_EQ(WriteExtentPattern(base), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WriteExtentPattern(base + sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WriteExtentPattern(base + 2 * sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, base, 2 * sz),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(HasExtentPattern(base), 0);
    WHAL_ASSERT_EQ(HasExtentPattern(base + sz), 0);
    WHAL_ASSERT_EQ(HasExtentPattern(base + 2 * sz), 1);

    /* Exactly one sector: the next sector is untouched */
    WHAL_ASSERT_EQ(WriteExtentPattern(base), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WriteExtentPattern(base + sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, base, sz),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(HasExtentPattern(base), 0);
    WHAL_ASSERT_EQ(HasExtentPattern(base + sz), 1);

    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, base, 3 * sz),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Lock(g_testFlashDev, base, 3 * sz),
                   WHAL_SUCCESS);
}

static void Test_Flash_EraseOneBytePastSector(void)
{
    const size_t sz = g_testFlashSectorSz;
    const uintptr_t base = ExtentBase();

    if (ExtentOverlapsImage(base))
        WHAL_SKIP();

    WHAL_ASSERT_EQ(whal_Flash_Unlock(g_testFlashDev, base, 3 * sz),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, base, 3 * sz),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(WriteExtentPattern(base), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WriteExtentPattern(base + sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WriteExtentPattern(base + 2 * sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, base, sz + 1),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(HasExtentPattern(base), 0);
    WHAL_ASSERT_EQ(HasExtentPattern(base + sz), 0);
    WHAL_ASSERT_EQ(HasExtentPattern(base + 2 * sz), 1);

    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, base, 3 * sz),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Lock(g_testFlashDev, base, 3 * sz),
                   WHAL_SUCCESS);
}

static void Test_Flash_CrossPageWrite(void)
{
    static uint8_t pattern[300];
    static uint8_t readback[300];
    const size_t page = g_testFlashPageSz;
    const struct {
        size_t offset;
        size_t len;
    } cases[] = {
        { 100, 300 },               /* crosses the first page boundary */
        { 2 * page - 8, page + 8 }, /* starts 8 bytes before a boundary */
        { 3 * page, page },         /* exactly one aligned page */
    };
    size_t i;
    size_t j;
    whal_Error err;

    /* The cases need a page that fits the buffers and four pages per sector */
    if (page == 0 || page + 8 > sizeof(pattern) ||
        4 * page > g_testFlashSectorSz)
        WHAL_SKIP();

    for (i = 0; i < sizeof(pattern); i++)
        pattern[i] = (uint8_t)(i * 7 + 3);

    WHAL_ASSERT_EQ(whal_Flash_Unlock(g_testFlashDev, g_testFlashAddr,
                                     g_testFlashSectorSz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, g_testFlashAddr,
                                    g_testFlashSectorSz), WHAL_SUCCESS);

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        size_t addr = g_testFlashAddr + cases[i].offset;

        do {
            err = whal_Flash_Write(g_testFlashDev, addr, pattern,
                                   cases[i].len);
        } while (err == WHAL_ENOTREADY);
        WHAL_ASSERT_EQ(err, WHAL_SUCCESS);

        for (j = 0; j < cases[i].len; j++)
            readback[j] = 0;
        WHAL_ASSERT_EQ(whal_Flash_Read(g_testFlashDev, addr, readback,
                                       cases[i].len), WHAL_SUCCESS);
        WHAL_ASSERT_MEM_EQ(readback, pattern, cases[i].len);
    }

    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, g_testFlashAddr,
                                    g_testFlashSectorSz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Lock(g_testFlashDev, g_testFlashAddr,
                                   g_testFlashSectorSz), WHAL_SUCCESS);
}

static void Test_Flash_OutOfBounds(void)
{
    const size_t start = BOARD_FLASH_START_ADDR;
    const size_t end = BOARD_FLASH_START_ADDR + BOARD_FLASH_SIZE;
    const size_t wrapSz = (size_t)0 - (end - 16) + 16;
    static uint8_t buf[32];

    /* Below the region */
    WHAL_ASSERT_EQ(whal_Flash_Read(g_testFlashDev, start - 16, buf, 16),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Write(g_testFlashDev, start - 16, buf, 16),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, start - 16, 16),
                   WHAL_EINVAL);

    /* Starting at the end of the region */
    WHAL_ASSERT_EQ(whal_Flash_Read(g_testFlashDev, end, buf, 16),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Write(g_testFlashDev, end, buf, 16),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, end, 16),
                   WHAL_EINVAL);

    /* Starting inside the region and running past the end */
    WHAL_ASSERT_EQ(whal_Flash_Read(g_testFlashDev, end - 16, buf, 32),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Write(g_testFlashDev, end - 16, buf, 32),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, end - 16, 32),
                   WHAL_EINVAL);

    /* Size large enough that addr + dataSz wraps around to 16 */
    WHAL_ASSERT_EQ(whal_Flash_Read(g_testFlashDev, end - 16, buf, wrapSz),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Write(g_testFlashDev, end - 16, buf, wrapSz),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(g_testFlashDev, end - 16, wrapSz),
                   WHAL_EINVAL);
}

static void run_flash_tests(const char *name, int onChip)
{
    g_testFlashOnChip = onChip;
    WHAL_TEST_SUITE_START("flash");
    if (name)
        whal_Test_Printf("  device: %s\n", name);
    WHAL_TEST(Test_Flash_Api);
    WHAL_TEST(Test_Flash_WriteReadErase);
    WHAL_TEST(Test_Flash_EraseExtent);
    if (onChip) {
        WHAL_TEST(Test_Flash_EraseOneBytePastSector);
        WHAL_TEST(Test_Flash_OutOfBounds);
    } else {
        WHAL_TEST(Test_Flash_CrossPageWrite);
    }
    WHAL_TEST_SUITE_END();
}

void whal_Test_Flash(void)
{
    /* Test on-chip flash */
    g_testFlashDev = BOARD_FLASH_DEV;
    g_testFlashStart = BOARD_FLASH_START_ADDR;
    g_testFlashAddr = BOARD_FLASH_TEST_ADDR;
    g_testFlashSectorSz = BOARD_FLASH_SECTOR_SZ;
    run_flash_tests("on-chip", 1);

    /* Test peripheral flash devices */
    for (size_t i = 0; g_peripheralFlash[i].dev; i++) {
        g_testFlashDev = g_peripheralFlash[i].dev;
        g_testFlashStart = 0;
        g_testFlashAddr = 0;
        g_testFlashSectorSz = g_peripheralFlash[i].sectorSz;
        g_testFlashPageSz = g_peripheralFlash[i].pageSz;
        run_flash_tests(g_peripheralFlash[i].name, 0);
    }
}
