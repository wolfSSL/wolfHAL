/* test_stm32wb_pwm.c
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
#include <wolfHAL/pwm/stm32wb_lptim_pwm.h>
#include "wolfHAL_board.h"
#include "test.h"

/* The LPTIM driver must enforce the same waveform contract when called
 * directly as it does through whal_Pwm_Start. Every case here returns before
 * any LPTIM register access, so no hardware state is touched. */
static void Test_Stm32wb_Lptim_Pwm_Reject(void)
{
    whal_Pwm_ChannelCfg wave = {
        .periodCycles = 1000,
        .pulseCycles  = 500,
        .pulseCount   = WHAL_PWM_PULSE_COUNT_CONTINUOUS,
        .polarity     = WHAL_PWM_POLARITY_NORMAL,
    };

    /* Structural waveform violations are EINVAL, matching whal_Pwm_Start. */
    wave.pulseCycles = wave.periodCycles + 1;
    WHAL_ASSERT_EQ(whal_Stm32wb_Lptim_Pwm_Start(BOARD_PWM_DEV,
                       WHAL_STM32WB_LPTIM_PWM_CHANNEL, &wave), WHAL_EINVAL);
    wave.pulseCycles = 500;

    wave.periodCycles = 0;
    WHAL_ASSERT_EQ(whal_Stm32wb_Lptim_Pwm_Start(BOARD_PWM_DEV,
                       WHAL_STM32WB_LPTIM_PWM_CHANNEL, &wave), WHAL_EINVAL);
    wave.periodCycles = 1000;

    /* Limits the LPTIM cannot represent are ENOTSUP. */
    WHAL_ASSERT_EQ(whal_Stm32wb_Lptim_Pwm_Start(BOARD_PWM_DEV, 1, &wave),
                   WHAL_ENOTSUP);

    wave.periodCycles = 0x10001; /* beyond the 16-bit ARR range */
    WHAL_ASSERT_EQ(whal_Stm32wb_Lptim_Pwm_Start(BOARD_PWM_DEV,
                       WHAL_STM32WB_LPTIM_PWM_CHANNEL, &wave), WHAL_ENOTSUP);
    wave.periodCycles = 1000;

    wave.pulseCount = 3; /* LPTIM has no pulse counter */
    WHAL_ASSERT_EQ(whal_Stm32wb_Lptim_Pwm_Start(BOARD_PWM_DEV,
                       WHAL_STM32WB_LPTIM_PWM_CHANNEL, &wave), WHAL_ENOTSUP);
}

/* Accepted boundaries: the largest representable period and 100% duty. Each
 * programs the LPTIM, so the channel is stopped again afterwards. */
static void Test_Stm32wb_Lptim_Pwm_Accept(void)
{
    whal_Pwm_ChannelCfg wave = {
        .pulseCount = WHAL_PWM_PULSE_COUNT_CONTINUOUS,
        .polarity   = WHAL_PWM_POLARITY_NORMAL,
    };

    /* Largest representable period (ARR = 0xFFFF). */
    wave.periodCycles = 0x10000;
    wave.pulseCycles  = 0x8000;
    WHAL_ASSERT_EQ(whal_Stm32wb_Lptim_Pwm_Start(BOARD_PWM_DEV,
                       WHAL_STM32WB_LPTIM_PWM_CHANNEL, &wave), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Stm32wb_Lptim_Pwm_Stop(BOARD_PWM_DEV,
                       WHAL_STM32WB_LPTIM_PWM_CHANNEL), WHAL_SUCCESS);

    /* Maximum duty: pulse == period exercises the CMP = 0 branch. */
    wave.periodCycles = 1000;
    wave.pulseCycles  = 1000;
    WHAL_ASSERT_EQ(whal_Stm32wb_Lptim_Pwm_Start(BOARD_PWM_DEV,
                       WHAL_STM32WB_LPTIM_PWM_CHANNEL, &wave), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Stm32wb_Lptim_Pwm_Stop(BOARD_PWM_DEV,
                       WHAL_STM32WB_LPTIM_PWM_CHANNEL), WHAL_SUCCESS);
}

void whal_Test_Pwm_Platform(void)
{
    WHAL_TEST_SUITE_START("pwm_platform");
    WHAL_TEST(Test_Stm32wb_Lptim_Pwm_Reject);
    WHAL_TEST(Test_Stm32wb_Lptim_Pwm_Accept);
    WHAL_TEST_SUITE_END();
}
