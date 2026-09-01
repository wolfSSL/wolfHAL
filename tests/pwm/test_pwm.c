/* test_pwm.c
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

static void Test_Pwm_Api(void)
{
    /* A waveform every driver can represent: channel 0, continuous, 25% duty. */
    whal_Pwm_ChannelCfg wave = {
        .periodCycles = 1000,
        .pulseCycles  = 250,
        .pulseCount   = WHAL_PWM_PULSE_COUNT_CONTINUOUS,
        .polarity     = WHAL_PWM_POLARITY_NORMAL,
    };

    /* Init ran in Board_Init; Start/Stop is a repeatable toggle, so run two cycles. */
    WHAL_ASSERT_EQ(whal_Pwm_Start(BOARD_PWM_DEV, 0, &wave), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Pwm_Stop(BOARD_PWM_DEV, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Pwm_Start(BOARD_PWM_DEV, 0, &wave), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Pwm_Stop(BOARD_PWM_DEV, 0), WHAL_SUCCESS);

    /* Structural violations the generic dispatch rejects: pulse > period, and null pointers. */
    wave.pulseCycles = wave.periodCycles + 1;
    WHAL_ASSERT_EQ(whal_Pwm_Start(BOARD_PWM_DEV, 0, &wave), WHAL_EINVAL);
    wave.pulseCycles = 250;

    WHAL_ASSERT_EQ(whal_Pwm_Start(NULL, 0, &wave), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Pwm_Start(BOARD_PWM_DEV, 0, NULL), WHAL_EINVAL);
}

void whal_Test_Pwm(void)
{
    WHAL_TEST_SUITE_START("pwm");
    WHAL_TEST(Test_Pwm_Api);
    WHAL_TEST_SUITE_END();
}
