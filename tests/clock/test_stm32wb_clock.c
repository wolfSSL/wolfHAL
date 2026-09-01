/* test_stm32wb_clock.c
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
#include <wolfHAL/platform/st/stm32wb55xx.h>
#include <wolfHAL/clock/stm32wb_rcc.h>
#include <wolfHAL/bitops.h>
#include "wolfHAL_board.h"
#include "test.h"

static void Test_Clock_EnableDisable(void)
{
    whal_Stm32wb_Rcc_PeriphClk testClk = { WHAL_STM32WB55_GPIOA_GATE };

    /* Save original state */
    size_t origVal = 0;
    whal_Reg_Get(WHAL_STM32WB_RCC_BASE, 0x4C, (1 << 0), 0, &origVal);

    /* Enable and verify */
    WHAL_ASSERT_EQ(whal_Stm32wb_Rcc_EnablePeriphClk(&testClk), WHAL_SUCCESS);

    size_t val = 0;
    whal_Reg_Get(WHAL_STM32WB_RCC_BASE, 0x4C, (1 << 0), 0, &val);
    WHAL_ASSERT_EQ(val, 1);

    /* Disable and verify */
    WHAL_ASSERT_EQ(whal_Stm32wb_Rcc_DisablePeriphClk(&testClk), WHAL_SUCCESS);

    whal_Reg_Get(WHAL_STM32WB_RCC_BASE, 0x4C, (1 << 0), 0, &val);
    WHAL_ASSERT_EQ(val, 0);

    /* Restore original state */
    if (origVal)
        whal_Stm32wb_Rcc_EnablePeriphClk(&testClk);
}

void whal_Test_Clock_Platform(void)
{
    WHAL_TEST(Test_Clock_EnableDisable);
}
