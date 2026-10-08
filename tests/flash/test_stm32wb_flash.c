/* test_stm32wb_flash.c
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
#include <wolfHAL/flash/stm32wb_flash.h>
#include <wolfHAL/bitops.h>
#include "wolfHAL_board.h"
#include "test.h"

/* Flash CR register offset and LOCK bit */
#define FLASH_CR_REG  0x14
#define FLASH_CR_LOCK_Pos 31
#define FLASH_CR_LOCK_Msk (1UL << FLASH_CR_LOCK_Pos)

static void Test_Flash_LockReadback(void)
{
    /* After locking, the CR.LOCK bit should be set */
    WHAL_ASSERT_EQ(whal_Flash_Lock(BOARD_FLASH_DEV, 0, 0), WHAL_SUCCESS);

    size_t val = 0;
    whal_Reg_Get(whal_Stm32wb_Flash_Dev.base, FLASH_CR_REG,
                 FLASH_CR_LOCK_Msk, FLASH_CR_LOCK_Pos, &val);
    WHAL_ASSERT_EQ(val, 1);
}

static void LockBlocksWrite(void)
{
    const uint8_t written[16] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    };
    const uint8_t attempted[16] = {
        0x5A, 0x5A, 0x5A, 0x5A, 0xA5, 0xA5, 0xA5, 0xA5,
        0x5A, 0x5A, 0x5A, 0x5A, 0xA5, 0xA5, 0xA5, 0xA5,
    };
    uint8_t erased[16];
    uint8_t readback[16];
    whal_Error err;
    size_t i;

    for (i = 0; i < sizeof(erased); i++)
        erased[i] = 0xFF;

    WHAL_ASSERT_EQ(whal_Flash_Unlock(BOARD_FLASH_DEV, 0, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                     BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
    do {
        err = whal_Flash_Write(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                               written, sizeof(written));
    } while (err == WHAL_ENOTREADY);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Flash_Lock(BOARD_FLASH_DEV, 0, 0), WHAL_SUCCESS);

    /* FLASH_CR is write-protected while locked, so PG stays clear and the
     * programming attempt raises PGSERR */
    WHAL_ASSERT_EQ(whal_Flash_Write(BOARD_FLASH_DEV,
                                     BOARD_FLASH_TEST_ADDR + sizeof(written),
                                     attempted, sizeof(attempted)),
                   WHAL_EHARDWARE);

    WHAL_ASSERT_EQ(whal_Flash_Read(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                    readback, sizeof(readback)), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(readback, written, sizeof(written));

    WHAL_ASSERT_EQ(whal_Flash_Read(BOARD_FLASH_DEV,
                                    BOARD_FLASH_TEST_ADDR + sizeof(written),
                                    readback, sizeof(readback)), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(readback, erased, sizeof(erased));
}

static void Test_Flash_LockBlocksWrite(void)
{
    LockBlocksWrite();

    /* Leave the test sector erased and the flash locked */
    whal_Flash_Unlock(BOARD_FLASH_DEV, 0, 0);
    whal_Flash_Erase(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                     BOARD_FLASH_SECTOR_SZ);
    whal_Flash_Lock(BOARD_FLASH_DEV, 0, 0);
}

void whal_Test_Flash_Platform(void)
{
    WHAL_TEST(Test_Flash_LockReadback);
    WHAL_TEST(Test_Flash_LockBlocksWrite);
}
