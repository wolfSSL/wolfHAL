/* test_pic32cz_gpio.c
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
#include <wolfHAL/gpio/pic32cz_gpio.h>
#include <wolfHAL/bitops.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * PIC32CZ PORT register offsets.
 * Each port group is 0x80 bytes. LED is on port B (index 1), pin 21.
 */
#define PORT_DIR_REG(port) (0x00 + ((port) * 0x80))
#define PORT_OUT_REG(port) (0x10 + ((port) * 0x80))

/* LED pin: port B, pin 21 */
#define LED_PORT 1
#define LED_HW_PIN 21

static void Test_Gpio_DirRegister(void)
{
    /* PB21 should be configured as output (bit 21 set in DIR register) */
    size_t val = 0;
    whal_Reg_Get(whal_Pic32cz_Gpio_Dev.base, PORT_DIR_REG(LED_PORT),
                 (1UL << LED_HW_PIN), LED_HW_PIN, &val);
    WHAL_ASSERT_EQ(val, 1);
}

static void Test_Gpio_SetHighReg(void)
{
    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1), WHAL_SUCCESS);

    /* Readback OUT register bit 21 */
    size_t val = 0;
    whal_Reg_Get(whal_Pic32cz_Gpio_Dev.base, PORT_OUT_REG(LED_PORT),
                 (1UL << LED_HW_PIN), LED_HW_PIN, &val);
    WHAL_ASSERT_EQ(val, 1);
}

static void Test_Gpio_SetLowReg(void)
{
    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 0), WHAL_SUCCESS);

    size_t val = 0;
    whal_Reg_Get(whal_Pic32cz_Gpio_Dev.base, PORT_OUT_REG(LED_PORT),
                 (1UL << LED_HW_PIN), LED_HW_PIN, &val);
    WHAL_ASSERT_EQ(val, 0);
}

void whal_Test_Gpio_Platform(void)
{
    WHAL_TEST(Test_Gpio_DirRegister);
    WHAL_TEST(Test_Gpio_SetHighReg);
    WHAL_TEST(Test_Gpio_SetLowReg);
}
