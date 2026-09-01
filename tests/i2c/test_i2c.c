/* test_i2c.c
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

static void Test_I2c_Api(void)
{
    WHAL_ASSERT_EQ(whal_I2c_StartCom(BOARD_I2C_DEV, NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_I2c_Transfer(BOARD_I2C_DEV, NULL, 1), WHAL_EINVAL);
}

void whal_Test_I2c(void)
{
    WHAL_TEST_SUITE_START("i2c");
    WHAL_TEST(Test_I2c_Api);
    WHAL_TEST_SUITE_END();
}
