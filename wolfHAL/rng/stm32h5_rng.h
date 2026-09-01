/* stm32h5_rng.h
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

#ifndef WHAL_STM32H5_RNG_H
#define WHAL_STM32H5_RNG_H

#include <stdint.h>
#include <wolfHAL/rng/rng.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32h5_rng.h
 * @brief STM32H5 RNG driver configuration.
 *
 * The STM32H5 true random number generator provides 32-bit random values
 * from an analog noise source with NIST SP800-90B conditioning. It features
 * a 4-word output FIFO and requires a CONDRST sequence to apply configuration
 * changes. This driver uses the NIST-certified configuration from AN4230.
 */

/*
 * @brief RNG device configuration.
 */
typedef struct whal_Stm32h5_Rng_Cfg {
    whal_Timeout *timeout;
} whal_Stm32h5_Rng_Cfg;

/*
 * @brief Platform-owned RNG device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32H5_RNG_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Rng whal_Stm32h5_Rng_Dev;

#ifndef WHAL_CFG_STM32H5_RNG_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32H5 RNG peripheral.
 */
extern const whal_RngDriver whal_Stm32h5_Rng_Driver;

/*
 * @brief Initialize the STM32H5 RNG peripheral.
 *
 * @param rngDev RNG device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Rng_Init(whal_Rng *rngDev);

/*
 * @brief Deinitialize the STM32H5 RNG peripheral.
 *
 * @param rngDev RNG device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Rng_Deinit(whal_Rng *rngDev);

/*
 * @brief Generate random data.
 *
 * Polls for DRDY and fills the output buffer. The RNG must be
 * initialized via Init() before calling this function.
 *
 * @param rngDev    RNG device instance.
 * @param rngData   Destination buffer.
 * @param rngDataSz Number of random bytes to generate.
 *
 * @retval WHAL_SUCCESS   Buffer filled with random data.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_EHARDWARE Seed or clock error detected.
 * @retval WHAL_ETIMEOUT  Timed out waiting for random data.
 */
whal_Error whal_Stm32h5_Rng_Generate(whal_Rng *rngDev, void *rngData,
                                     size_t rngDataSz);
#endif /* !WHAL_CFG_STM32H5_RNG_DIRECT_API_MAPPING */

#endif /* WHAL_STM32H5_RNG_H */
