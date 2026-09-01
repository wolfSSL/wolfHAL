/* test_gpio.c
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
#include "wolfHAL_board.h"
#include "test.h"

static void Test_Gpio_Api(void)
{
    WHAL_ASSERT_EQ(whal_Gpio_Get(BOARD_GPIO_DEV, 0, NULL), WHAL_EINVAL);
}

static void Test_Gpio_SetGetHighLow(void)
{
    size_t val = 0;

    /* Set high and verify */
    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Gpio_Get(BOARD_GPIO_DEV, BOARD_LED_PIN, &val), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val, 1);

    /* Set low and verify */
    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Gpio_Get(BOARD_GPIO_DEV, BOARD_LED_PIN, &val), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val, 0);
}

void whal_Test_Gpio(void)
{
    WHAL_TEST_SUITE_START("gpio");
    WHAL_TEST(Test_Gpio_Api);
    WHAL_TEST(Test_Gpio_SetGetHighLow);
    WHAL_TEST_SUITE_END();
}
