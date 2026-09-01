/* test_watchdog.c
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
#include <wolfHAL/watchdog/watchdog.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * Multi-phase watchdog test.
 *
 * Phase 0: Write a magic value to flash, start the watchdog, refresh
 *          it several times to verify refresh works, then stop
 *          refreshing and let the watchdog reset the system.
 * Phase 1: After reboot, read flash and verify the magic value is
 *          present, confirming the watchdog fired. Erase the flash
 *          sector to clean up.
 *
 * Uses the board's flash test area (BOARD_FLASH_TEST_ADDR) to persist
 * state across resets. Flash survives all reset types including
 * power-on reset.
 */

#define WDG_TEST_MAGIC      0x57444F47  /* "WDOG" */
#define WDG_TEST_REFRESH_OK 0x524F4B21  /* "ROK!" */

typedef struct {
    uint32_t magic;
    uint32_t refreshOk;
    uint32_t pad[2];
} wdg_test_state_t;

static void Test_Watchdog(void)
{
    wdg_test_state_t state;

    WHAL_ASSERT_EQ(whal_Flash_Read(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                    &state, sizeof(state)), WHAL_SUCCESS);

    if (state.magic != WDG_TEST_MAGIC) {
        /* Phase 0: write magic, init watchdog, refresh, then let it reset */
        whal_Error err;

        state.magic = WDG_TEST_MAGIC;
        state.refreshOk = 0;
        state.pad[0] = 0;
        state.pad[1] = 0;

        WHAL_ASSERT_EQ(whal_Flash_Unlock(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                          BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
        WHAL_ASSERT_EQ(whal_Flash_Erase(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                         BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
        do {
            err = whal_Flash_Write(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                    &state, sizeof(state));
        } while (err == WHAL_ENOTREADY);
        WHAL_ASSERT_EQ(err, WHAL_SUCCESS);

        WHAL_ASSERT_EQ(whal_Watchdog_Init(BOARD_WATCHDOG_DEV), WHAL_SUCCESS);

        /* Verify refresh keeps us alive */
        for (int i = 0; i < 1000; i++) {
            Board_WaitMs(1);
            WHAL_ASSERT_EQ(whal_Watchdog_Refresh(BOARD_WATCHDOG_DEV),
                           WHAL_SUCCESS);
        }

        /* Mark that refresh loop completed */
        state.refreshOk = WDG_TEST_REFRESH_OK;
        WHAL_ASSERT_EQ(whal_Flash_Erase(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                         BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
        do {
            err = whal_Flash_Write(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                    &state, sizeof(state));
        } while (err == WHAL_ENOTREADY);
        WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
        WHAL_ASSERT_EQ(whal_Flash_Lock(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                       BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);

        /* Stop refreshing — watchdog should reset us */
        while (1);
    }

    /* Phase 1: watchdog fired — clean up first, then verify */
    uint32_t refreshOk = state.refreshOk;
    WHAL_ASSERT_EQ(whal_Flash_Unlock(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                      BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                     BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Lock(BOARD_FLASH_DEV, BOARD_FLASH_TEST_ADDR,
                                   BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(refreshOk, WDG_TEST_REFRESH_OK);
}

void whal_Test_Watchdog(void)
{
    WHAL_TEST_SUITE_START("watchdog");
    WHAL_TEST(Test_Watchdog);
    WHAL_TEST_SUITE_END();
}
