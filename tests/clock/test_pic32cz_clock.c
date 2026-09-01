/* test_pic32cz_clock.c
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
#include <wolfHAL/clock/pic32cz_clock.h>
#include <wolfHAL/bitops.h>
#include "wolfHAL_board.h"
#include "test.h"

#define GCLK_PCHCTRL_OFFSET(ch) (0x10000 + 0x80 + ((ch) * 4))
#define GCLK_PCHCTRL_CHEN_Pos   6
#define GCLK_PCHCTRL_CHEN_Msk   (1UL << GCLK_PCHCTRL_CHEN_Pos)

#define MCLK_CLKMSK_OFFSET(inst) (0x12000 + 0x3C + ((inst) * 4))

static void Test_Clock_EnableDisable(void)
{
    /* Use a test clock descriptor for SERCOM4 (same as UART clock) */
    whal_Pic32cz_Clock_PeriphClk testClk = {
        .gclkPeriphChannel = 25,
        .gclkPeriphSrc = 0,
        .mclkEnableInst = 1,
        .mclkEnableMask = (1UL << 3),
        .mclkEnablePos = 3,
    };

    /* Save original state */
    size_t origChen = 0;
    whal_Reg_Get(WHAL_PIC32CZ_CLOCK_BASE, GCLK_PCHCTRL_OFFSET(25),
                 GCLK_PCHCTRL_CHEN_Msk, GCLK_PCHCTRL_CHEN_Pos, &origChen);

    size_t origMclk = 0;
    whal_Reg_Get(WHAL_PIC32CZ_CLOCK_BASE, MCLK_CLKMSK_OFFSET(1),
                 (1UL << 3), 3, &origMclk);

    size_t val = 0;

    /* Enable and verify */
    WHAL_ASSERT_EQ(whal_Pic32cz_Clock_EnablePeriphClk(&testClk), WHAL_SUCCESS);

    whal_Reg_Get(WHAL_PIC32CZ_CLOCK_BASE, GCLK_PCHCTRL_OFFSET(25),
                 GCLK_PCHCTRL_CHEN_Msk, GCLK_PCHCTRL_CHEN_Pos, &val);
    WHAL_ASSERT_EQ(val, 1);

    whal_Reg_Get(WHAL_PIC32CZ_CLOCK_BASE, MCLK_CLKMSK_OFFSET(1),
                 (1UL << 3), 3, &val);
    WHAL_ASSERT_EQ(val, 1);

    /* Disable and verify */
    WHAL_ASSERT_EQ(whal_Pic32cz_Clock_DisablePeriphClk(&testClk), WHAL_SUCCESS);

    whal_Reg_Get(WHAL_PIC32CZ_CLOCK_BASE, GCLK_PCHCTRL_OFFSET(25),
                 GCLK_PCHCTRL_CHEN_Msk, GCLK_PCHCTRL_CHEN_Pos, &val);
    WHAL_ASSERT_EQ(val, 0);

    /* Restore original state */
    if (origChen)
        whal_Pic32cz_Clock_EnablePeriphClk(&testClk);
}

void whal_Test_Clock_Platform(void)
{
    WHAL_TEST(Test_Clock_EnableDisable);
}
