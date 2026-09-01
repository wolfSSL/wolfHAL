/* stm32wb_gpio.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32WB_GPIO_DEV initializer */
#include <wolfHAL/error.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/gpio/stm32wb_gpio.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

const whal_Gpio whal_Stm32wb_Gpio_Dev = WHAL_CFG_STM32WB_GPIO_DEV;

/*
 * STM32WB GPIO Register Definitions
 *
 * Each GPIO port has a 0x400 byte register block. Register offsets
 * are relative to the port base address.
 */

/* Size of each GPIO port register block */
#define GPIO_PORT_SIZE 0x400

/* Mode register - 2 bits per pin, selects input/output/altfn/analog */
#define GPIO_MODE_REG     0x00
/* Output type register - 1 bit per pin, push-pull or open-drain */
#define GPIO_OUTTYPE_REG  0x04
/* Output speed register - 2 bits per pin */
#define GPIO_SPEED_REG    0x08
/* Pull-up/pull-down register - 2 bits per pin */
#define GPIO_PULL_REG     0x0C
/* Input data register - read-only, 1 bit per pin */
#define GPIO_IDR_REG      0x10
/* Output data register - 1 bit per pin */
#define GPIO_ODR_REG      0x14
/* Alternate function low register - 4 bits per pin for pins 0-7 */
#define GPIO_ALTFNL_REG   0x20
/* Alternate function high register - 4 bits per pin for pins 8-15 */
#define GPIO_ALTFNH_REG   0x24

#ifdef WHAL_CFG_STM32WB_GPIO_DIRECT_API_MAPPING
#define whal_Stm32wb_Gpio_Init   whal_Gpio_Init
#define whal_Stm32wb_Gpio_Deinit whal_Gpio_Deinit
#define whal_Stm32wb_Gpio_Get    whal_Gpio_Get
#define whal_Stm32wb_Gpio_Set    whal_Gpio_Set
#endif /* WHAL_CFG_GPIO_API_MAPPING */

/*
 * Configure alternate function for a pin.
 *
 * The AFRL (pins 0-7) and AFRH (pins 8-15) registers are combined into
 * a 64-bit value for easier manipulation. Each pin uses 4 bits to select
 * one of 16 alternate functions (AF0-AF15).
 */
/*
 * Initialize a single GPIO pin with the specified configuration.
 */
static inline whal_Error whal_Stm32wb_Gpio_InitPin(whal_Stm32wb_Gpio_PinCfg cfg)
{
    uint8_t pin = WHAL_STM32WB_GPIO_GET_PIN(cfg);
    size_t portBase;
    uint8_t pos2;
    size_t mask2, mask1;

    if (pin > 15)
        return WHAL_EINVAL;

    portBase = whal_Stm32wb_Gpio_Dev.base +
               (WHAL_STM32WB_GPIO_GET_PORT(cfg) * GPIO_PORT_SIZE);
    pos2 = pin << 1;
    mask2 = WHAL_BITMASK(2) << pos2;
    mask1 = 1UL << pin;

    whal_Reg_Update(portBase, GPIO_MODE_REG, mask2,
                    WHAL_STM32WB_GPIO_GET_MODE(cfg) << pos2);

    whal_Reg_Update(portBase, GPIO_SPEED_REG, mask2,
                    WHAL_STM32WB_GPIO_GET_SPEED(cfg) << pos2);

    whal_Reg_Update(portBase, GPIO_PULL_REG, mask2,
                    WHAL_STM32WB_GPIO_GET_PULL(cfg) << pos2);

    whal_Reg_Update(portBase, GPIO_OUTTYPE_REG, mask1,
                    WHAL_STM32WB_GPIO_GET_OUTTYPE(cfg) << pin);

    if (WHAL_STM32WB_GPIO_GET_MODE(cfg) == WHAL_STM32WB_GPIO_MODE_ALTFN) {
        uint8_t afPos = (pin & 7) << 2;

        whal_Reg_Update(portBase,
                        (pin < 8) ? GPIO_ALTFNL_REG : GPIO_ALTFNH_REG,
                        WHAL_BITMASK(4) << afPos,
                        WHAL_STM32WB_GPIO_GET_ALTFN(cfg) << afPos);
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Gpio_Init(whal_Gpio *gpioDev)
{
    const whal_Stm32wb_Gpio_Cfg *cfg =
        (const whal_Stm32wb_Gpio_Cfg *)whal_Stm32wb_Gpio_Dev.cfg;
    const whal_Stm32wb_Gpio_PinCfg *pinCfg = cfg->pinCfg;
    whal_Error err;
    (void)gpioDev;

    /* Initialize each pin in the configuration array */
    for (size_t pin = 0; pin < cfg->pinCount; ++pin) {
        err = whal_Stm32wb_Gpio_InitPin(pinCfg[pin]);
        if (err) {
            return err;
        }
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Gpio_Deinit(whal_Gpio *gpioDev)
{
    (void)gpioDev;
    return WHAL_SUCCESS;
}

/*
 * Shared helper for reading or writing GPIO pin values.
 *
 * For set operations (set=1): writes value to ODR register
 * For get operations (set=0): reads value from IDR register
 */
static whal_Error whal_Stm32wb_Gpio_SetOrGet(whal_Gpio *gpioDev, size_t idx,
                                             size_t *value, uint8_t set)
{
    const whal_Stm32wb_Gpio_Cfg *cfg =
        (const whal_Stm32wb_Gpio_Cfg *)whal_Stm32wb_Gpio_Dev.cfg;
    whal_Stm32wb_Gpio_PinCfg pinCfg;
    uint8_t port, pin;
    size_t portBase, mask;
    (void)gpioDev;

    if (!value) {
        return WHAL_EINVAL;
    }

    pinCfg = cfg->pinCfg[idx];
    port = WHAL_STM32WB_GPIO_GET_PORT(pinCfg);
    pin = WHAL_STM32WB_GPIO_GET_PIN(pinCfg);

    if (pin > 15) {
        return WHAL_EINVAL;
    }

    portBase = whal_Stm32wb_Gpio_Dev.base + (port * GPIO_PORT_SIZE);
    mask = 1UL << pin;

    if (set) {
        whal_Reg_Update(portBase, GPIO_ODR_REG, mask,
                        whal_SetBits(mask, pin, *value));
    } else {
        whal_Reg_Get(portBase, GPIO_IDR_REG, mask, pin, value);
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Gpio_Get(whal_Gpio *gpioDev, size_t pin, size_t *value)
{
    return whal_Stm32wb_Gpio_SetOrGet(gpioDev, pin, value, 0);
}

whal_Error whal_Stm32wb_Gpio_Set(whal_Gpio *gpioDev, size_t pin, size_t value)
{
    return whal_Stm32wb_Gpio_SetOrGet(gpioDev, pin, &value, 1);
}

#ifndef WHAL_CFG_STM32WB_GPIO_DIRECT_API_MAPPING
const whal_GpioDriver whal_Stm32wb_Gpio_Driver = {
    .Init = whal_Stm32wb_Gpio_Init,
    .Deinit = whal_Stm32wb_Gpio_Deinit,
    .Get = whal_Stm32wb_Gpio_Get,
    .Set = whal_Stm32wb_Gpio_Set,
};
#endif /* !WHAL_CFG_GPIO_API_MAPPING */
