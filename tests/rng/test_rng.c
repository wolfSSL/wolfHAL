/* test_rng.c
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

static void Test_Rng_Api(void)
{
    WHAL_ASSERT_EQ(whal_Rng_Generate(BOARD_RNG_DEV, NULL, 8), WHAL_EINVAL);
}

static void Test_Rng_GenerateNonzero(void)
{
    uint8_t buf[32] = {0};
    int allZero = 1;

    WHAL_ASSERT_EQ(whal_Rng_Generate(BOARD_RNG_DEV, buf, sizeof(buf)), WHAL_SUCCESS);

    for (size_t i = 0; i < sizeof(buf); i++) {
        if (buf[i] != 0) {
            allZero = 0;
            break;
        }
    }

    WHAL_ASSERT_EQ(allZero, 0);
}

static void Test_Rng_GenerateUnique(void)
{
    uint8_t buf1[16] = {0};
    uint8_t buf2[16] = {0};
    int same = 1;

    WHAL_ASSERT_EQ(whal_Rng_Generate(BOARD_RNG_DEV, buf1, sizeof(buf1)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Rng_Generate(BOARD_RNG_DEV, buf2, sizeof(buf2)), WHAL_SUCCESS);

    for (size_t i = 0; i < sizeof(buf1); i++) {
        if (buf1[i] != buf2[i]) {
            same = 0;
            break;
        }
    }

    /* Two consecutive 16-byte outputs should differ */
    WHAL_ASSERT_EQ(same, 0);
}

void whal_Test_Rng(void)
{
    WHAL_TEST_SUITE_START("rng");
    WHAL_TEST(Test_Rng_Api);
    WHAL_TEST(Test_Rng_GenerateNonzero);
    WHAL_TEST(Test_Rng_GenerateUnique);
    WHAL_TEST_SUITE_END();
}
