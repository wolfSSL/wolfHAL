/* rng.h
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

#ifndef WHAL_RNG_H
#define WHAL_RNG_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>

/*
 * @file rng.h
 * @brief Generic RNG abstraction and driver interface.
 */

typedef struct whal_Rng whal_Rng;

/*
 * @brief Driver vtable for RNG devices.
 */
typedef struct {
    /* Initialize the RNG hardware. */
    whal_Error (*Init)(whal_Rng *rngDev);
    /* Deinitialize the RNG hardware. */
    whal_Error (*Deinit)(whal_Rng *rngDev);
    /* Generate random data into a buffer. */
    whal_Error (*Generate)(whal_Rng *rngDev, void *rngData, size_t rngDataSz);
} whal_RngDriver;

/*
 * @brief RNG device instance with a base address, driver vtable, and platform-specific cfg.
 */
struct whal_Rng {
    const uintptr_t base;
    const whal_RngDriver *driver;
    void *cfg;
};

/*
 * @brief Initialize an RNG device and its driver.
 *
 * @param rngDev RNG instance to initialize.
 *
 * @retval WHAL_SUCCESS Driver-specific init completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Rng_Init(whal_Rng *rngDev);
/*
 * @brief Deinitialize an RNG device and release resources.
 *
 * @param rngDev RNG instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Driver-specific deinit completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Rng_Deinit(whal_Rng *rngDev);
/*
 * @brief Generate random data into a buffer.
 *
 * @param rngDev    RNG instance to use.
 * @param rngData   Destination buffer.
 * @param rngDataSz Number of random bytes to generate.
 *
 * @retval WHAL_SUCCESS Buffer filled with random data.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Rng_Generate(whal_Rng *rngDev, void *rngData, size_t rngDataSz);

#endif /* WHAL_RNG_H */
