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

void whal_Test_Flash_Platform(void)
{
    WHAL_TEST(Test_Flash_LockReadback);
}
