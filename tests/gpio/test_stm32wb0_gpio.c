/* test_stm32wb0_gpio.c
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
#include <wolfHAL/gpio/stm32wb0_gpio.h>
#include <wolfHAL/bitops.h>
#include "wolfHAL_board.h"
#include "test.h"

/* STM32WB0 ports are 0x100000 apart; the per-port offsets match other STM32
 * families. */
#define GPIOx_MODE_REG    0x00
#define GPIOx_OTYPER_REG  0x04
#define GPIOx_OSPEEDR_REG 0x08
#define GPIOx_PUPDR_REG   0x0C
#define GPIOx_ODR_REG     0x14
#define GPIOx_AFRL_REG    0x20
#define GPIOx_AFRH_REG    0x24
#define GPIOx_STRIDE      0x100000

static const whal_Stm32wb0_Gpio_Cfg *GpioCfg(void)
{
    return (const whal_Stm32wb0_Gpio_Cfg *)whal_Stm32wb0_Gpio_Dev.cfg;
}

static uintptr_t PortBase(whal_Stm32wb0_Gpio_PinCfg p)
{
    return whal_Stm32wb0_Gpio_Dev.base +
           WHAL_STM32WB0_GPIO_GET_PORT(p) * GPIOx_STRIDE;
}

/* Every pin in the board table reads back with its configured mode, output
 * type, speed, pull and, for alternate-function pins, AF number. */
static void Test_Gpio_PinTableRegisters(void)
{
    const whal_Stm32wb0_Gpio_Cfg *cfg = GpioCfg();

    for (size_t i = 0; i < cfg->pinCount; i++) {
        whal_Stm32wb0_Gpio_PinCfg p = cfg->pinCfg[i];
        uintptr_t base = PortBase(p);
        size_t pin = WHAL_STM32WB0_GPIO_GET_PIN(p);
        size_t pos2 = pin << 1;
        uint32_t val = 0;

        whal_Reg32_Get(base, GPIOx_MODE_REG, WHAL_BITMASK(2) << pos2, pos2,
                       &val);
        WHAL_ASSERT_EQ(val, WHAL_STM32WB0_GPIO_GET_MODE(p));

        whal_Reg32_Get(base, GPIOx_OTYPER_REG, 1UL << pin, pin, &val);
        WHAL_ASSERT_EQ(val, WHAL_STM32WB0_GPIO_GET_OUTTYPE(p));

        whal_Reg32_Get(base, GPIOx_OSPEEDR_REG, WHAL_BITMASK(2) << pos2, pos2,
                       &val);
        WHAL_ASSERT_EQ(val, WHAL_STM32WB0_GPIO_GET_SPEED(p));

        whal_Reg32_Get(base, GPIOx_PUPDR_REG, WHAL_BITMASK(2) << pos2, pos2,
                       &val);
        WHAL_ASSERT_EQ(val, WHAL_STM32WB0_GPIO_GET_PULL(p));

        if (WHAL_STM32WB0_GPIO_GET_MODE(p) == WHAL_STM32WB0_GPIO_MODE_ALTFN) {
            size_t afPos = (pin & 7) << 2;

            whal_Reg32_Get(base, (pin < 8) ? GPIOx_AFRL_REG : GPIOx_AFRH_REG,
                           WHAL_BITMASK(4) << afPos, afPos, &val);
            WHAL_ASSERT_EQ(val, WHAL_STM32WB0_GPIO_GET_ALTFN(p));
        }
    }
}

static void Test_Gpio_SetLedReg(void)
{
    whal_Stm32wb0_Gpio_PinCfg led = GpioCfg()->pinCfg[BOARD_LED_PIN];
    size_t pin = WHAL_STM32WB0_GPIO_GET_PIN(led);
    uint32_t val = 0;

    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1),
                   WHAL_SUCCESS);
    whal_Reg32_Get(PortBase(led), GPIOx_ODR_REG, 1UL << pin, pin, &val);
    WHAL_ASSERT_EQ(val, 1);

    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 0),
                   WHAL_SUCCESS);
    whal_Reg32_Get(PortBase(led), GPIOx_ODR_REG, 1UL << pin, pin, &val);
    WHAL_ASSERT_EQ(val, 0);
}

void whal_Test_Gpio_Platform(void)
{
    WHAL_TEST(Test_Gpio_PinTableRegisters);
    WHAL_TEST(Test_Gpio_SetLedReg);
}
