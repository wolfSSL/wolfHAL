/* stm32wb_rng.h
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

#ifndef WHAL_STM32WB_RNG_H
#define WHAL_STM32WB_RNG_H

#include <stdint.h>
#include <wolfHAL/rng/rng.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32wb_rng.h
 * @brief STM32WB RNG driver configuration.
 *
 * The STM32WB true random number generator provides 32-bit random values
 * from an analog noise source. The peripheral requires the RNG clock to
 * be enabled and produces one 32-bit word at a time via the DR register.
 */

/*
 * @brief RNG device configuration.
 */
typedef struct whal_Stm32wb_Rng_Cfg {
    whal_Timeout *timeout;
} whal_Stm32wb_Rng_Cfg;

/*
 * @brief Platform-owned RNG device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32WB_RNG_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Rng whal_Stm32wb_Rng_Dev;

#ifndef WHAL_CFG_STM32WB_RNG_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32WB RNG peripheral.
 */
extern const whal_RngDriver whal_Stm32wb_Rng_Driver;

/*
 * @brief Initialize the STM32WB RNG peripheral.
 *
 * @param rngDev RNG device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Rng_Init(whal_Rng *rngDev);
/*
 * @brief Deinitialize the STM32WB RNG peripheral.
 *
 * @param rngDev RNG device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Rng_Deinit(whal_Rng *rngDev);
/*
 * @brief Generate random data.
 *
 * @param rngDev    RNG device instance.
 * @param rngData   Destination buffer.
 * @param rngDataSz Number of random bytes to generate.
 *
 * @retval WHAL_SUCCESS Buffer filled with random data.
 * @retval WHAL_EINVAL  Invalid arguments or seed/clock error detected.
 */
whal_Error whal_Stm32wb_Rng_Generate(whal_Rng *rngDev, void *rngData, size_t rngDataSz);
#endif /* !WHAL_CFG_STM32WB_RNG_DIRECT_API_MAPPING */

#endif /* WHAL_STM32WB_RNG_H */
