/* test_stm32wb0_flash.c
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
#include <wolfHAL/flash/stm32wb0_flash.h>
#include "wolfHAL_board.h"
#include "test.h"

/* RM0505 does not say whether a command on a PAGEPROT-protected page raises
 * CMDERR or is ignored, so the test checks the flash contents rather than the
 * return code. */

static const uint8_t g_pattern[16] = {
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xF0, 0x0F,
};

static whal_Error WritePattern(size_t addr)
{
    whal_Error err;

    do {
        err = whal_Flash_Write(BOARD_FLASH_DEV, addr, g_pattern,
                               sizeof(g_pattern));
    } while (err == WHAL_ENOTREADY);

    return err;
}

static int IsErased(size_t addr, size_t len)
{
    uint8_t buf[16];

    if (len > sizeof(buf) ||
        whal_Flash_Read(BOARD_FLASH_DEV, addr, buf, len) != WHAL_SUCCESS)
        return 0;

    for (size_t i = 0; i < len; i++) {
        if (buf[i] != 0xFF)
            return 0;
    }
    return 1;
}

static int HasPattern(size_t addr)
{
    uint8_t buf[sizeof(g_pattern)];

    if (whal_Flash_Read(BOARD_FLASH_DEV, addr, buf, sizeof(buf)) !=
        WHAL_SUCCESS)
        return 0;

    for (size_t i = 0; i < sizeof(buf); i++) {
        if (buf[i] != g_pattern[i])
            return 0;
    }
    return 1;
}

static void Test_Stm32wb0_Flash_LockBlocksWrite(void)
{
    const size_t addr = BOARD_FLASH_TEST_ADDR;
    const size_t sz = BOARD_FLASH_SECTOR_SZ;
    const size_t blank = addr + 64;
    whal_Error err;
    int patternKept;
    int blankKept;

    WHAL_ASSERT_EQ(whal_Flash_Unlock(BOARD_FLASH_DEV, addr, sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(BOARD_FLASH_DEV, addr, sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WritePattern(addr), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Lock(BOARD_FLASH_DEV, addr, sz), WHAL_SUCCESS);

    /* Neither an erase nor a write may change a locked page */
    err = whal_Flash_Erase(BOARD_FLASH_DEV, addr, sz);
    WHAL_ASSERT_EQ(err == WHAL_SUCCESS || err == WHAL_EHARDWARE, 1);
    err = WritePattern(blank);
    WHAL_ASSERT_EQ(err == WHAL_SUCCESS || err == WHAL_EHARDWARE, 1);

    patternKept = HasPattern(addr);
    blankKept = IsErased(blank, sizeof(g_pattern));

    /* Leave the test page erased and locked */
    WHAL_ASSERT_EQ(whal_Flash_Unlock(BOARD_FLASH_DEV, addr, sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(BOARD_FLASH_DEV, addr, sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Lock(BOARD_FLASH_DEV, addr, sz), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(patternKept, 1);
    WHAL_ASSERT_EQ(blankKept, 1);
}

void whal_Test_Flash_Platform(void)
{
    WHAL_TEST_SUITE_START("flash_platform");
    WHAL_TEST(Test_Stm32wb0_Flash_LockBlocksWrite);
    WHAL_TEST_SUITE_END();
}
