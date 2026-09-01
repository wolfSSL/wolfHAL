/* stm32wb0_gpio.h
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

#ifndef WHAL_STM32WB0_GPIO_H
#define WHAL_STM32WB0_GPIO_H

#include <stdint.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/reg.h>

/**
 * @file stm32wb0_gpio.h
 * @brief STM32WB0 GPIO driver.
 *
 * The STM32WB0 GPIO peripheral has the same per-port register layout as
 * the rest of the STM32 family (MODER/OTYPER/OSPEEDR/PUPDR/IDR/ODR/
 * BSRR/AFRL/AFRH at offsets 0x00..0x24, 2-bit-per-pin encoding) but a
 * very different inter-port address spacing: GPIOA at 0x48000000,
 * GPIOB at 0x48100000 (1 MB apart, not the usual 0x400). Only ports
 * A and B exist.
 *
 * Because the wider STM32WB GPIO driver indexes ports off
 * base + (port * 0x400), it cannot be aliased for WB0. This is a
 * dedicated driver that uses the same pin packing and field encodings
 * but the WB0-specific port spacing.
 */

/**
 * @brief Packed per-pin GPIO configuration (uint32_t).
 *
 * Bit layout matches the wider STM32 driver to keep the pinCfg helpers
 * uniform across platforms:
 *   [3:0]   port    — 0 (port A) or 1 (port B)
 *   [7:4]   pin     — Pin number 0..15
 *   [9:8]   mode    — Pin mode (00 in, 01 out, 10 altfn, 11 analog)
 *   [10]    outType — Output type (0 push-pull, 1 open-drain)
 *   [12:11] speed   — Output speed
 *   [14:13] pull    — Pull resistor
 *   [18:15] altFn   — Alternate function 0..7 on WB0
 */
typedef uint32_t whal_Stm32wb0_Gpio_PinCfg;

#define WHAL_STM32WB0_GPIO_PORT_Pos    0
#define WHAL_STM32WB0_GPIO_PIN_Pos     4
#define WHAL_STM32WB0_GPIO_MODE_Pos    8
#define WHAL_STM32WB0_GPIO_OUTTYPE_Pos 10
#define WHAL_STM32WB0_GPIO_SPEED_Pos   11
#define WHAL_STM32WB0_GPIO_PULL_Pos    13
#define WHAL_STM32WB0_GPIO_ALTFN_Pos   15

#define WHAL_STM32WB0_GPIO_PORT_A 0
#define WHAL_STM32WB0_GPIO_PORT_B 1

#define WHAL_STM32WB0_GPIO_MODE_IN     0
#define WHAL_STM32WB0_GPIO_MODE_OUT    1
#define WHAL_STM32WB0_GPIO_MODE_ALTFN  2
#define WHAL_STM32WB0_GPIO_MODE_ANALOG 3

#define WHAL_STM32WB0_GPIO_OUTTYPE_PUSHPULL  0
#define WHAL_STM32WB0_GPIO_OUTTYPE_OPENDRAIN 1

#define WHAL_STM32WB0_GPIO_SPEED_LOW    0
#define WHAL_STM32WB0_GPIO_SPEED_MEDIUM 1
#define WHAL_STM32WB0_GPIO_SPEED_FAST   2
#define WHAL_STM32WB0_GPIO_SPEED_HIGH   3

#define WHAL_STM32WB0_GPIO_PULL_NONE 0
#define WHAL_STM32WB0_GPIO_PULL_UP   1
#define WHAL_STM32WB0_GPIO_PULL_DOWN 2

/**
 * @brief Pack a pin configuration into a uint32_t.
 */
#define WHAL_STM32WB0_GPIO_PIN(port, pin, mode, outType, speed, pull, altFn) \
    ((((uint32_t)(port)    & 0xFu) << WHAL_STM32WB0_GPIO_PORT_Pos)    | \
     (((uint32_t)(pin)     & 0xFu) << WHAL_STM32WB0_GPIO_PIN_Pos)     | \
     (((uint32_t)(mode)    & 0x3u) << WHAL_STM32WB0_GPIO_MODE_Pos)    | \
     (((uint32_t)(outType) & 0x1u) << WHAL_STM32WB0_GPIO_OUTTYPE_Pos) | \
     (((uint32_t)(speed)   & 0x3u) << WHAL_STM32WB0_GPIO_SPEED_Pos)   | \
     (((uint32_t)(pull)    & 0x3u) << WHAL_STM32WB0_GPIO_PULL_Pos)    | \
     (((uint32_t)(altFn)   & 0xFu) << WHAL_STM32WB0_GPIO_ALTFN_Pos))

#define WHAL_STM32WB0_GPIO_GET_PORT(cfg)    (((cfg) >> WHAL_STM32WB0_GPIO_PORT_Pos)    & 0xF)
#define WHAL_STM32WB0_GPIO_GET_PIN(cfg)     (((cfg) >> WHAL_STM32WB0_GPIO_PIN_Pos)     & 0xF)
#define WHAL_STM32WB0_GPIO_GET_MODE(cfg)    (((cfg) >> WHAL_STM32WB0_GPIO_MODE_Pos)    & 0x3)
#define WHAL_STM32WB0_GPIO_GET_OUTTYPE(cfg) (((cfg) >> WHAL_STM32WB0_GPIO_OUTTYPE_Pos) & 0x1)
#define WHAL_STM32WB0_GPIO_GET_SPEED(cfg)   (((cfg) >> WHAL_STM32WB0_GPIO_SPEED_Pos)   & 0x3)
#define WHAL_STM32WB0_GPIO_GET_PULL(cfg)    (((cfg) >> WHAL_STM32WB0_GPIO_PULL_Pos)    & 0x3)
#define WHAL_STM32WB0_GPIO_GET_ALTFN(cfg)   (((cfg) >> WHAL_STM32WB0_GPIO_ALTFN_Pos)   & 0xF)

/**
 * @brief GPIO device configuration.
 */
typedef struct {
    const whal_Stm32wb0_Gpio_PinCfg *pinCfg;
    size_t pinCount;
} whal_Stm32wb0_Gpio_Cfg;

/**
 * @brief Platform-owned GPIO device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32WB0_GPIO_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Gpio whal_Stm32wb0_Gpio_Dev;

#ifndef WHAL_CFG_STM32WB0_GPIO_DIRECT_API_MAPPING
extern const whal_GpioDriver whal_Stm32wb0_Gpio_Driver;

whal_Error whal_Stm32wb0_Gpio_Init(whal_Gpio *gpioDev);
whal_Error whal_Stm32wb0_Gpio_Deinit(whal_Gpio *gpioDev);
whal_Error whal_Stm32wb0_Gpio_Get(whal_Gpio *gpioDev, size_t pin, size_t *value);
whal_Error whal_Stm32wb0_Gpio_Set(whal_Gpio *gpioDev, size_t pin, size_t value);
#endif /* !WHAL_CFG_STM32WB0_GPIO_DIRECT_API_MAPPING */

#endif /* WHAL_STM32WB0_GPIO_H */
