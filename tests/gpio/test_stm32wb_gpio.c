/* test_stm32wb_gpio.c
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
#include <wolfHAL/gpio/stm32wb_gpio.h>
#include <wolfHAL/bitops.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * GPIO register offsets. Port and pin are derived from the LED entry in
 * whal_Stm32wb_Gpio_Dev.cfg->pinCfg[BOARD_LED_PIN]. Ports are
 * WHAL_STM32WB_GPIO_PORT_SIZE apart.
 */
#define GPIOx_MODE_REG    0x00
#define GPIOx_OTYPER_REG  0x04
#define GPIOx_OSPEEDR_REG 0x08
#define GPIOx_PUPDR_REG   0x0C
#define GPIOx_ODR_REG     0x14
#define GPIOx_AFRL_REG    0x20
#define GPIOx_AFRH_REG    0x24

static inline size_t Board_LedPortBase(void)
{
    const whal_Stm32wb_Gpio_Cfg *cfg = (const whal_Stm32wb_Gpio_Cfg *)whal_Stm32wb_Gpio_Dev.cfg;
    whal_Stm32wb_Gpio_PinCfg led = cfg->pinCfg[BOARD_LED_PIN];
    return whal_Stm32wb_Gpio_Dev.base + WHAL_STM32WB_GPIO_GET_PORT(led) * WHAL_STM32WB_GPIO_PORT_SIZE;
}

static inline size_t Board_LedPinNum(void)
{
    const whal_Stm32wb_Gpio_Cfg *cfg = (const whal_Stm32wb_Gpio_Cfg *)whal_Stm32wb_Gpio_Dev.cfg;
    whal_Stm32wb_Gpio_PinCfg led = cfg->pinCfg[BOARD_LED_PIN];
    return WHAL_STM32WB_GPIO_GET_PIN(led);
}

static void Test_Gpio_PinCfgRoundTrip(void)
{
    whal_Stm32wb_Gpio_PinCfg cfg = WHAL_STM32WB_GPIO_PIN(
        WHAL_STM32WB_GPIO_PORT_C, 13, WHAL_STM32WB_GPIO_MODE_ALTFN,
        WHAL_STM32WB_GPIO_OUTTYPE_OPENDRAIN, WHAL_STM32WB_GPIO_SPEED_HIGH,
        WHAL_STM32WB_GPIO_PULL_DOWN, 9);

    WHAL_ASSERT_EQ(WHAL_STM32WB_GPIO_GET_PORT(cfg), WHAL_STM32WB_GPIO_PORT_C);
    WHAL_ASSERT_EQ(WHAL_STM32WB_GPIO_GET_PIN(cfg), 13);
    WHAL_ASSERT_EQ(WHAL_STM32WB_GPIO_GET_MODE(cfg), WHAL_STM32WB_GPIO_MODE_ALTFN);
    WHAL_ASSERT_EQ(WHAL_STM32WB_GPIO_GET_OUTTYPE(cfg), WHAL_STM32WB_GPIO_OUTTYPE_OPENDRAIN);
    WHAL_ASSERT_EQ(WHAL_STM32WB_GPIO_GET_SPEED(cfg), WHAL_STM32WB_GPIO_SPEED_HIGH);
    WHAL_ASSERT_EQ(WHAL_STM32WB_GPIO_GET_PULL(cfg), WHAL_STM32WB_GPIO_PULL_DOWN);
    WHAL_ASSERT_EQ(WHAL_STM32WB_GPIO_GET_ALTFN(cfg), 9);
}

static void Test_Gpio_NoDuplicatePins(void)
{
    const whal_Stm32wb_Gpio_Cfg *cfg = (const whal_Stm32wb_Gpio_Cfg *)whal_Stm32wb_Gpio_Dev.cfg;
    const whal_Stm32wb_Gpio_PinCfg *pins = cfg->pinCfg;

    for (size_t i = 0; i < cfg->pinCount; i++) {
        for (size_t j = i + 1; j < cfg->pinCount; j++) {
            if (WHAL_STM32WB_GPIO_GET_PORT(pins[i]) == WHAL_STM32WB_GPIO_GET_PORT(pins[j]) &&
                WHAL_STM32WB_GPIO_GET_PIN(pins[i]) == WHAL_STM32WB_GPIO_GET_PIN(pins[j])) {
                WHAL_ASSERT_NEQ(WHAL_STM32WB_GPIO_GET_PORT(pins[i]),
                                WHAL_STM32WB_GPIO_GET_PORT(pins[j]));
            }
        }
    }
}

static void Test_Gpio_ModeRegister(void)
{
    size_t pinNum = Board_LedPinNum();
    size_t bitPos = pinNum << 1;
    size_t mask = (WHAL_BITMASK(2) << bitPos);
    size_t val = 0;

    whal_Reg_Get(Board_LedPortBase(), GPIOx_MODE_REG, mask, bitPos, &val);
    WHAL_ASSERT_EQ(val, WHAL_STM32WB_GPIO_MODE_OUT);
}

/* Every pin in the board table reads back with its configured mode, output
 * type, speed, pull and, for alternate-function pins, AF number. */
static void Test_Gpio_PinTableRegisters(void)
{
    const whal_Stm32wb_Gpio_Cfg *cfg = (const whal_Stm32wb_Gpio_Cfg *)whal_Stm32wb_Gpio_Dev.cfg;

    for (size_t i = 0; i < cfg->pinCount; i++) {
        whal_Stm32wb_Gpio_PinCfg p = cfg->pinCfg[i];
        size_t base = whal_Stm32wb_Gpio_Dev.base +
                      WHAL_STM32WB_GPIO_GET_PORT(p) * WHAL_STM32WB_GPIO_PORT_SIZE;
        size_t pin = WHAL_STM32WB_GPIO_GET_PIN(p);
        size_t pos2 = pin << 1;
        size_t val = 0;

        whal_Reg_Get(base, GPIOx_MODE_REG, WHAL_BITMASK(2) << pos2, pos2,
                     &val);
        WHAL_ASSERT_EQ(val, WHAL_STM32WB_GPIO_GET_MODE(p));

        whal_Reg_Get(base, GPIOx_OTYPER_REG, 1UL << pin, pin, &val);
        WHAL_ASSERT_EQ(val, WHAL_STM32WB_GPIO_GET_OUTTYPE(p));

        whal_Reg_Get(base, GPIOx_OSPEEDR_REG, WHAL_BITMASK(2) << pos2, pos2,
                     &val);
        WHAL_ASSERT_EQ(val, WHAL_STM32WB_GPIO_GET_SPEED(p));

        whal_Reg_Get(base, GPIOx_PUPDR_REG, WHAL_BITMASK(2) << pos2, pos2,
                     &val);
        WHAL_ASSERT_EQ(val, WHAL_STM32WB_GPIO_GET_PULL(p));

        if (WHAL_STM32WB_GPIO_GET_MODE(p) == WHAL_STM32WB_GPIO_MODE_ALTFN) {
            size_t afPos = (pin & 7) << 2;

            whal_Reg_Get(base, (pin < 8) ? GPIOx_AFRL_REG : GPIOx_AFRH_REG,
                         WHAL_BITMASK(4) << afPos, afPos, &val);
            WHAL_ASSERT_EQ(val, WHAL_STM32WB_GPIO_GET_ALTFN(p));
        }
    }
}

static void Test_Gpio_SetHighReg(void)
{
    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1), WHAL_SUCCESS);

    size_t pinNum = Board_LedPinNum();
    size_t val = 0;
    whal_Reg_Get(Board_LedPortBase(), GPIOx_ODR_REG, (1UL << pinNum),
                 pinNum, &val);
    WHAL_ASSERT_EQ(val, 1);
}

static void Test_Gpio_SetLowReg(void)
{
    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 0), WHAL_SUCCESS);

    size_t pinNum = Board_LedPinNum();
    size_t val = 0;
    whal_Reg_Get(Board_LedPortBase(), GPIOx_ODR_REG, (1UL << pinNum),
                 pinNum, &val);
    WHAL_ASSERT_EQ(val, 0);
}

static void Test_Gpio_SetNonzeroReg(void)
{
    size_t pinNum = Board_LedPinNum();
    size_t val = 0;

    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 2), WHAL_SUCCESS);
    whal_Reg_Get(Board_LedPortBase(), GPIOx_ODR_REG, (1UL << pinNum),
                 pinNum, &val);
    WHAL_ASSERT_EQ(val, 1);

    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 0), WHAL_SUCCESS);
}

void whal_Test_Gpio_Platform(void)
{
    WHAL_TEST(Test_Gpio_PinCfgRoundTrip);
    WHAL_TEST(Test_Gpio_NoDuplicatePins);
    WHAL_TEST(Test_Gpio_ModeRegister);
    WHAL_TEST(Test_Gpio_PinTableRegisters);
    WHAL_TEST(Test_Gpio_SetHighReg);
    WHAL_TEST(Test_Gpio_SetLowReg);
    WHAL_TEST(Test_Gpio_SetNonzeroReg);
}
