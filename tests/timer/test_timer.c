/* test_timer.c
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
#include <wolfHAL/timer/timer.h>
#include "wolfHAL_board.h"
#include "test.h"

static void Test_Timer_TicksAdvance(void)
{
    size_t before = g_tick;

    /* Spin for ~100ms worth of ticks. At 1ms/tick this should yield ~100. */
    while (g_tick - before < 100) {}

    size_t elapsed = g_tick - before;

    /* At least 100 ticks should have passed */
    WHAL_ASSERT_NEQ(elapsed, 0);
    WHAL_ASSERT_EQ(elapsed >= 100, 1);
}

void whal_Test_Timer(void)
{
    WHAL_TEST_SUITE_START("timer");
    WHAL_TEST(Test_Timer_TicksAdvance);
    WHAL_TEST_SUITE_END();
}
