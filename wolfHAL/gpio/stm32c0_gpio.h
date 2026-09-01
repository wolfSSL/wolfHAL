/* stm32c0_gpio.h
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

#ifndef WHAL_STM32C0_GPIO_H
#define WHAL_STM32C0_GPIO_H

/*
 * @file stm32c0_gpio.h
 * @brief STM32C0 GPIO driver (alias for STM32WB GPIO).
 *
 * The STM32C0 GPIO peripheral is register-compatible with the STM32WB GPIO.
 * This header re-exports the STM32WB GPIO driver types and symbols under
 * STM32C0-specific names. The underlying implementation is shared.
 */

#include <wolfHAL/gpio/stm32wb_gpio.h>

typedef whal_Stm32wb_Gpio_Cfg    whal_Stm32c0_Gpio_Cfg;
typedef whal_Stm32wb_Gpio_PinCfg whal_Stm32c0_Gpio_PinCfg;

#define whal_Stm32c0_Gpio_Dev whal_Stm32wb_Gpio_Dev

#ifndef WHAL_CFG_STM32C0_GPIO_DIRECT_API_MAPPING
#define whal_Stm32c0_Gpio_Driver whal_Stm32wb_Gpio_Driver
#define whal_Stm32c0_Gpio_Init   whal_Stm32wb_Gpio_Init
#define whal_Stm32c0_Gpio_Deinit whal_Stm32wb_Gpio_Deinit
#define whal_Stm32c0_Gpio_Get    whal_Stm32wb_Gpio_Get
#define whal_Stm32c0_Gpio_Set    whal_Stm32wb_Gpio_Set
#endif /* !WHAL_CFG_STM32C0_GPIO_DIRECT_API_MAPPING */

/*
 * @brief GPIO mode selection (re-exported from STM32WB).
 */
#define WHAL_STM32C0_GPIO_MODE_IN    WHAL_STM32WB_GPIO_MODE_IN
#define WHAL_STM32C0_GPIO_MODE_OUT   WHAL_STM32WB_GPIO_MODE_OUT
#define WHAL_STM32C0_GPIO_MODE_ALTFN WHAL_STM32WB_GPIO_MODE_ALTFN
#define WHAL_STM32C0_GPIO_MODE_ANALOG WHAL_STM32WB_GPIO_MODE_ANALOG

/*
 * @brief GPIO output type (re-exported from STM32WB).
 */
#define WHAL_STM32C0_GPIO_OUTTYPE_PUSHPULL  WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL
#define WHAL_STM32C0_GPIO_OUTTYPE_OPENDRAIN WHAL_STM32WB_GPIO_OUTTYPE_OPENDRAIN

/*
 * @brief GPIO speed selection (re-exported from STM32WB).
 */
#define WHAL_STM32C0_GPIO_SPEED_LOW    WHAL_STM32WB_GPIO_SPEED_LOW
#define WHAL_STM32C0_GPIO_SPEED_MEDIUM WHAL_STM32WB_GPIO_SPEED_MEDIUM
#define WHAL_STM32C0_GPIO_SPEED_FAST   WHAL_STM32WB_GPIO_SPEED_FAST
#define WHAL_STM32C0_GPIO_SPEED_HIGH   WHAL_STM32WB_GPIO_SPEED_HIGH

/*
 * @brief GPIO pull-up/pull-down selection (re-exported from STM32WB).
 */
#define WHAL_STM32C0_GPIO_PULL_NONE WHAL_STM32WB_GPIO_PULL_NONE
#define WHAL_STM32C0_GPIO_PULL_UP   WHAL_STM32WB_GPIO_PULL_UP
#define WHAL_STM32C0_GPIO_PULL_DOWN WHAL_STM32WB_GPIO_PULL_DOWN

/*
 * @brief GPIO port selection (re-exported from STM32WB).
 */
#define WHAL_STM32C0_GPIO_PORT_A WHAL_STM32WB_GPIO_PORT_A
#define WHAL_STM32C0_GPIO_PORT_B WHAL_STM32WB_GPIO_PORT_B
#define WHAL_STM32C0_GPIO_PORT_C WHAL_STM32WB_GPIO_PORT_C
#define WHAL_STM32C0_GPIO_PORT_D WHAL_STM32WB_GPIO_PORT_D
#define WHAL_STM32C0_GPIO_PORT_F WHAL_STM32WB_GPIO_PORT_F

/*
 * @brief Pack a GPIO pin configuration into a single word (re-exported from
 * STM32WB).
 */
#define WHAL_STM32C0_GPIO_PIN WHAL_STM32WB_GPIO_PIN

/* Config initializer macro alias. The C0 wolfHAL_board.h supplies the body
 * under the C0-prefixed name; the WB driver source consumes the WB name. */
#define WHAL_CFG_STM32WB_GPIO_DEV WHAL_CFG_STM32C0_GPIO_DEV

#endif /* WHAL_STM32C0_GPIO_H */
