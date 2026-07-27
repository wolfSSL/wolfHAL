/* esp32c6_gpio.c
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

#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/gpio/esp32c6_gpio.h>
#include <wolfHAL/bitops.h>

#ifdef WHAL_CFG_ESP32C6_GPIO_SINGLE_INSTANCE
#include "board.h"  /* provides whal_Esp32c6_Gpio_Dev singleton */
#endif

/* GPIO matrix + IO MUX register offsets (TRM chapter 7).
 * GPIO_*   offsets are relative to WHAL_ESP32C6_GPIO_BASE  (0x60091000).
 * IO_MUX_* offsets are relative to WHAL_ESP32C6_IOMUX_BASE (0x60090000).
 * Field macros for composing register values live in esp32c6_gpio.h. */

/* Per-bit data / enable registers (bit n corresponds to GPIOn). */
#define GPIO_OUT_REG                 0x0004
#define GPIO_OUT_W1TS_REG            0x0008
#define GPIO_OUT_W1TC_REG            0x000C
#define GPIO_ENABLE_REG              0x0020
#define GPIO_ENABLE_W1TS_REG         0x0024
#define GPIO_ENABLE_W1TC_REG         0x0028
#define GPIO_IN_REG                  0x003C

/* IO MUX is a separate peripheral block from the GPIO block. */
#define IO_MUX_BASE                  0x60090000

/* Per-pin register offsets (n = GPIO pin). */
#define GPIO_PIN_REG(n)              (0x0074 + 4 * (n))
#define GPIO_FUNC_OUT_SEL_CFG_REG(n) (0x0554 + 4 * (n))
#define IO_MUX_GPIO_REG(n)           (0x0004 + 4 * (n))

/* GPIO matrix input routing offset (n = peripheral signal). Field macros for
 * composing the register value live in esp32c6_gpio.h. */
#define GPIO_FUNC_IN_SEL_CFG_REG(n)  (0x0154 + 4 * (n))

#ifdef WHAL_CFG_ESP32C6_GPIO_DIRECT_API_MAPPING
#define whal_Esp32c6_Gpio_Init   whal_Gpio_Init
#define whal_Esp32c6_Gpio_Deinit whal_Gpio_Deinit
#define whal_Esp32c6_Gpio_Get    whal_Gpio_Get
#define whal_Esp32c6_Gpio_Set    whal_Gpio_Set
#endif

#ifdef WHAL_CFG_ESP32C6_GPIO_SINGLE_INSTANCE
const whal_Gpio whal_Esp32c6_Gpio_Dev = WHAL_CFG_ESP32C6_GPIO_DEV;
#endif

static inline whal_Error whal_Esp32c6_Gpio_ConfigurePin(const size_t base, const whal_Esp32c6_Gpio_PinCfg *pinCfg)
{
    if (!pinCfg || pinCfg->pin > 30) {
        return WHAL_EINVAL;
    }

    whal_Reg_Write(base, GPIO_PIN_REG(pinCfg->pin), pinCfg->gpioCfg);
    whal_Reg_Write(IO_MUX_BASE, IO_MUX_GPIO_REG(pinCfg->pin), pinCfg->ioMuxCfg);

    if (pinCfg->outputEnable) {
        whal_Reg_Write(base, GPIO_ENABLE_W1TS_REG, 1UL << pinCfg->pin);
    }

    return WHAL_SUCCESS;
}

static inline whal_Error whal_Esp32c6_Gpio_ConfigureInput(const size_t base, const whal_Esp32c6_Gpio_InputCfg *inputCfg)
{
    if (!inputCfg || inputCfg->signalNum > 127) {
        return WHAL_EINVAL;
    }
    
    whal_Reg_Write(base, GPIO_FUNC_IN_SEL_CFG_REG(inputCfg->signalNum), inputCfg->cfg);

    return WHAL_SUCCESS;
}

static whal_Error whal_Esp32c6_Gpio_ConfigureOutput(const size_t base, const whal_Esp32c6_Gpio_OutputCfg *outputCfg)
{
    if (!outputCfg || outputCfg->gpioNum > 30) {
        return WHAL_EINVAL;
    }

    whal_Reg_Write(base, GPIO_FUNC_OUT_SEL_CFG_REG(outputCfg->gpioNum), outputCfg->cfg);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Gpio_Init(whal_Gpio *gpioDev)
{
    whal_Error err;
#ifdef WHAL_CFG_ESP32C6_GPIO_SINGLE_INSTANCE
    const whal_Esp32c6_Gpio_Cfg *cfg =
        (const whal_Esp32c6_Gpio_Cfg *)whal_Esp32c6_Gpio_Dev.cfg;
    size_t base = whal_Esp32c6_Gpio_Dev.base;
    (void)gpioDev;
#else
    const whal_Esp32c6_Gpio_Cfg *cfg;
    size_t base;

    if (!gpioDev || !gpioDev->cfg) {
        return WHAL_EINVAL;
    }

    base = gpioDev->base;
    cfg = (const whal_Esp32c6_Gpio_Cfg *)gpioDev->cfg;
#endif

    for (size_t i = 0; i < cfg->pinCount; ++i) {
        err = whal_Esp32c6_Gpio_ConfigurePin(base, &cfg->pinCfg[i]);
        if (err) {
            return err;
        }
    }

    for (size_t i = 0; i < cfg->inputCount; ++i) {
        err = whal_Esp32c6_Gpio_ConfigureInput(base, &cfg->inputCfg[i]);
        if (err) {
            return err;
        }
    }

    for (size_t i = 0; i < cfg->outputCount; ++i) {
        err = whal_Esp32c6_Gpio_ConfigureOutput(base, &cfg->outputCfg[i]);
        if (err) {
            return err;
        }
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Gpio_Deinit(whal_Gpio *gpioDev)
{
#ifdef WHAL_CFG_ESP32C6_GPIO_SINGLE_INSTANCE
    size_t base = whal_Esp32c6_Gpio_Dev.base;
    (void)gpioDev;
#else
    size_t base;

    if (!gpioDev) {
        return WHAL_EINVAL;
    }

    base = gpioDev->base;
#endif

    (void)base;
    return WHAL_ENOTSUP;
}

whal_Error whal_Esp32c6_Gpio_Get(whal_Gpio *gpioDev, size_t pin, size_t *value)
{
    const whal_Esp32c6_Gpio_PinCfg *pinCfg;
#ifdef WHAL_CFG_ESP32C6_GPIO_SINGLE_INSTANCE
    const whal_Esp32c6_Gpio_Cfg *cfg =
        (const whal_Esp32c6_Gpio_Cfg *)whal_Esp32c6_Gpio_Dev.cfg;
    size_t base = whal_Esp32c6_Gpio_Dev.base;
    (void)gpioDev;
#else
    const whal_Esp32c6_Gpio_Cfg *cfg;
    size_t base;

    if (!gpioDev || !gpioDev->cfg) {
        return WHAL_EINVAL;
    }

    base = gpioDev->base;
    cfg = (const whal_Esp32c6_Gpio_Cfg *)gpioDev->cfg;
#endif

    if (!value || pin >= cfg->pinCount) {
        return WHAL_EINVAL;
    }

    pinCfg = &cfg->pinCfg[pin];
    whal_Reg_Get(base, GPIO_IN_REG, 1UL << pinCfg->pin, pinCfg->pin, value);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Gpio_Set(whal_Gpio *gpioDev, size_t pin, size_t value)
{
    const whal_Esp32c6_Gpio_PinCfg *pinCfg;
#ifdef WHAL_CFG_ESP32C6_GPIO_SINGLE_INSTANCE
    const whal_Esp32c6_Gpio_Cfg *cfg =
        (const whal_Esp32c6_Gpio_Cfg *)whal_Esp32c6_Gpio_Dev.cfg;
    size_t base = whal_Esp32c6_Gpio_Dev.base;
    (void)gpioDev;
#else
    const whal_Esp32c6_Gpio_Cfg *cfg;
    size_t base;

    if (!gpioDev || !gpioDev->cfg) {
        return WHAL_EINVAL;
    }

    base = gpioDev->base;
    cfg = (const whal_Esp32c6_Gpio_Cfg *)gpioDev->cfg;
#endif

    if (pin >= cfg->pinCount) {
        return WHAL_EINVAL;
    }

    pinCfg = &cfg->pinCfg[pin];

    if (value) {
        whal_Reg_Write(base, GPIO_OUT_W1TS_REG, 1UL << pinCfg->pin);
    } else {
        whal_Reg_Write(base, GPIO_OUT_W1TC_REG, 1UL << pinCfg->pin);
    }

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_ESP32C6_GPIO_DIRECT_API_MAPPING
const whal_GpioDriver whal_Esp32c6_Gpio_Driver = {
    .Init = whal_Esp32c6_Gpio_Init,
    .Deinit = whal_Esp32c6_Gpio_Deinit,
    .Get = whal_Esp32c6_Gpio_Get,
    .Set = whal_Esp32c6_Gpio_Set,
};
#endif /* !WHAL_CFG_ESP32C6_GPIO_DIRECT_API_MAPPING */
