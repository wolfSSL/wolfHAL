/* stm32wb_gpio.h
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

#ifndef WHAL_STM32WB_GPIO_H
#define WHAL_STM32WB_GPIO_H

#include <stdint.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/reg.h>

/*
 * @file stm32wb_gpio.h
 * @brief STM32WB GPIO driver configuration types.
 *
 * The STM32WB GPIO peripheral provides:
 * - Up to 8 GPIO ports (A-H), each with up to 16 pins
 * - Configurable pin modes: input, output, alternate function, analog
 * - Output types: push-pull or open-drain
 * - Configurable output speed for EMI/power tradeoff
 * - Internal pull-up and pull-down resistors
 * - Alternate function mapping for peripheral connections (UART, SPI, etc.)
 *
 * Each port occupies 0x400 bytes in the memory map starting from GPIOA base.
 */

/*
 * @brief Packed per-pin GPIO configuration (uint32_t).
 *
 * Bit layout:
 *   [3:0]   port    — GPIO port A-I (4 bits)
 *   [7:4]   pin     — Pin number 0-15 (4 bits)
 *   [9:8]   mode    — Pin mode (2 bits)
 *   [10]    outType — Output type (1 bit)
 *   [12:11] speed   — Output speed (2 bits)
 *   [14:13] pull    — Pull resistor (2 bits)
 *   [18:15] altFn   — Alternate function 0-15 (4 bits)
 *
 * Use WHAL_STM32WB_GPIO_PIN() to build values and
 * WHAL_STM32WB_GPIO_GET_*() to extract fields.
 */
typedef uint32_t whal_Stm32wb_Gpio_PinCfg;

/* Field positions */
#define WHAL_STM32WB_GPIO_PORT_Pos      0
#define WHAL_STM32WB_GPIO_PIN_Pos       4
#define WHAL_STM32WB_GPIO_MODE_Pos      8
#define WHAL_STM32WB_GPIO_OUTTYPE_Pos   10
#define WHAL_STM32WB_GPIO_SPEED_Pos     11
#define WHAL_STM32WB_GPIO_PULL_Pos      13
#define WHAL_STM32WB_GPIO_ALTFN_Pos     15

/* Port values */
#define WHAL_STM32WB_GPIO_PORT_A    0
#define WHAL_STM32WB_GPIO_PORT_B    1
#define WHAL_STM32WB_GPIO_PORT_C    2
#define WHAL_STM32WB_GPIO_PORT_D    3
#define WHAL_STM32WB_GPIO_PORT_E    4
#define WHAL_STM32WB_GPIO_PORT_F    5
#define WHAL_STM32WB_GPIO_PORT_G    6
#define WHAL_STM32WB_GPIO_PORT_H    7
#define WHAL_STM32WB_GPIO_PORT_I    8

/* Mode values */
#define WHAL_STM32WB_GPIO_MODE_IN       0
#define WHAL_STM32WB_GPIO_MODE_OUT      1
#define WHAL_STM32WB_GPIO_MODE_ALTFN    2
#define WHAL_STM32WB_GPIO_MODE_ANALOG   3

/* Output type values */
#define WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL   0
#define WHAL_STM32WB_GPIO_OUTTYPE_OPENDRAIN  1

/* Speed values */
#define WHAL_STM32WB_GPIO_SPEED_LOW     0
#define WHAL_STM32WB_GPIO_SPEED_MEDIUM  1
#define WHAL_STM32WB_GPIO_SPEED_FAST    2
#define WHAL_STM32WB_GPIO_SPEED_HIGH    3

/* Pull values */
#define WHAL_STM32WB_GPIO_PULL_NONE     0
#define WHAL_STM32WB_GPIO_PULL_UP       1
#define WHAL_STM32WB_GPIO_PULL_DOWN     2

/* Pack a pin configuration into a uint32_t */
#define WHAL_STM32WB_GPIO_PIN(port, pin, mode, outType, speed, pull, altFn) \
    ((((uint32_t)(port)    & 0xFu) << WHAL_STM32WB_GPIO_PORT_Pos)    | \
     (((uint32_t)(pin)     & 0xFu) << WHAL_STM32WB_GPIO_PIN_Pos)     | \
     (((uint32_t)(mode)    & 0x3u) << WHAL_STM32WB_GPIO_MODE_Pos)    | \
     (((uint32_t)(outType) & 0x1u) << WHAL_STM32WB_GPIO_OUTTYPE_Pos) | \
     (((uint32_t)(speed)   & 0x3u) << WHAL_STM32WB_GPIO_SPEED_Pos)   | \
     (((uint32_t)(pull)    & 0x3u) << WHAL_STM32WB_GPIO_PULL_Pos)    | \
     (((uint32_t)(altFn)   & 0xFu) << WHAL_STM32WB_GPIO_ALTFN_Pos))

/* Extract individual fields */
#define WHAL_STM32WB_GPIO_GET_PORT(cfg)    (((cfg) >> WHAL_STM32WB_GPIO_PORT_Pos) & 0xF)
#define WHAL_STM32WB_GPIO_GET_PIN(cfg)     (((cfg) >> WHAL_STM32WB_GPIO_PIN_Pos) & 0xF)
#define WHAL_STM32WB_GPIO_GET_MODE(cfg)    (((cfg) >> WHAL_STM32WB_GPIO_MODE_Pos) & 0x3)
#define WHAL_STM32WB_GPIO_GET_OUTTYPE(cfg) (((cfg) >> WHAL_STM32WB_GPIO_OUTTYPE_Pos) & 0x1)
#define WHAL_STM32WB_GPIO_GET_SPEED(cfg)   (((cfg) >> WHAL_STM32WB_GPIO_SPEED_Pos) & 0x3)
#define WHAL_STM32WB_GPIO_GET_PULL(cfg)    (((cfg) >> WHAL_STM32WB_GPIO_PULL_Pos) & 0x3)
#define WHAL_STM32WB_GPIO_GET_ALTFN(cfg)   (((cfg) >> WHAL_STM32WB_GPIO_ALTFN_Pos) & 0xF)

/*
 * @brief GPIO device configuration.
 *
 * Contains clock control references and an array of pin configurations.
 */
typedef struct {
    const whal_Stm32wb_Gpio_PinCfg *pinCfg; /* Array of pin configurations */
    size_t pinCount;                        /* Number of pins to configure */
} whal_Stm32wb_Gpio_Cfg;

/*
 * @brief Platform-owned GPIO device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32WB_GPIO_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Gpio whal_Stm32wb_Gpio_Dev;

#if !defined(WHAL_CFG_STM32WB_GPIO_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32F4_GPIO_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32H5_GPIO_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32C0_GPIO_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32F0_GPIO_DIRECT_API_MAPPING)
/*
 * @brief Driver instance for STM32 GPIO.
 */
extern const whal_GpioDriver whal_Stm32wb_Gpio_Driver;

/*
 * @brief Initialize the STM32 GPIO peripheral and configured pins.
 *
 * @param gpioDev GPIO device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Gpio_Init(whal_Gpio *gpioDev);
/*
 * @brief Deinitialize the STM32 GPIO peripheral.
 *
 * @param gpioDev GPIO device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Gpio_Deinit(whal_Gpio *gpioDev);
/*
 * @brief Read a GPIO pin value.
 *
 * @param gpioDev GPIO device instance.
 * @param pin     Pin index in the configured pin table.
 * @param value   Output for the sampled pin value.
 *
 * @retval WHAL_SUCCESS Pin value read.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Gpio_Get(whal_Gpio *gpioDev, size_t pin, size_t *value);
/*
 * @brief Set a GPIO pin value.
 *
 * @param gpioDev GPIO device instance.
 * @param pin     Pin index in the configured pin table.
 * @param value   Value to drive.
 *
 * @retval WHAL_SUCCESS Pin updated.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Gpio_Set(whal_Gpio *gpioDev, size_t pin, size_t value);
#endif /* !WHAL_CFG_GPIO_API_MAPPING */

#endif /* WHAL_STM32WB_GPIO_H */
