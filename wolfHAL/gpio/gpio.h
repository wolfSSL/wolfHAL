/* gpio.h
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

#ifndef WHAL_GPIO_H
#define WHAL_GPIO_H

#include <wolfHAL/error.h>
#include <stddef.h>
#include <stdint.h>

/*
 * @file gpio.h
 * @brief Generic GPIO abstraction and driver interface.
 */

typedef struct whal_Gpio whal_Gpio;

/*
 * @brief Driver vtable for GPIO devices.
 */
typedef struct {
    /* Initialize GPIO hardware and configured pins. */
    whal_Error (*Init)(whal_Gpio *gpioDev); 
    /* Deinitialize GPIO hardware. */
    whal_Error (*Deinit)(whal_Gpio *gpioDev); 
    /* Read a pin value. */
    whal_Error (*Get)(whal_Gpio *gpioDev, size_t pin, size_t *value); 
    /* Write a pin value. */
    whal_Error (*Set)(whal_Gpio *gpioDev, size_t pin, size_t value); 
} whal_GpioDriver;

/*
 * @brief GPIO device instance containing driver and configuration data.
 */
struct whal_Gpio {
    const uintptr_t base;
    const whal_GpioDriver *driver;
    const void *cfg;
};

/*
 * @brief Initialize a GPIO device and its pins.
 *
 * @param gpioDev GPIO instance to initialize.
 *
 * @retval WHAL_SUCCESS Driver-specific init completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Gpio_Init(whal_Gpio *gpioDev);
/*
 * @brief Deinitialize a GPIO device.
 *
 * @param gpioDev GPIO instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Driver-specific deinit completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Gpio_Deinit(whal_Gpio *gpioDev);
/*
 * @brief Read the state of a pin.
 *
 * @param gpioDev GPIO instance containing the pin.
 * @param pin     Pin index into the configured pin table.
 * @param value   Storage for the sampled pin value.
 *
 * @retval WHAL_SUCCESS Pin value stored in @p value.
 * @retval WHAL_EINVAL   Null pointer or bad pin.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Gpio_Get(whal_Gpio *gpioDev, size_t pin, size_t *value);
/*
 * @brief Set the state of a pin.
 *
 * @param gpioDev GPIO instance containing the pin.
 * @param pin     Pin index into the configured pin table.
 * @param value   Output value to drive; any nonzero value drives the pin high.
 *
 * @retval WHAL_SUCCESS Pin updated.
 * @retval WHAL_EINVAL   Null pointer or bad pin.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Gpio_Set(whal_Gpio *gpioDev, size_t pin, size_t value);

#endif /* WHAL_GPIO_H */
