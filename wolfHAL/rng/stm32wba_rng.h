/* stm32wba_rng.h
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

#ifndef WHAL_STM32WBA_RNG_H
#define WHAL_STM32WBA_RNG_H

#include <wolfHAL/rng/rng.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32wba_rng.h
 * @brief STM32WBA RNG driver configuration.
 *
 * The RNG kernel clock source must be selected in RCC_CCIPR2.RNGSEL
 * before using the RNG. Default after reset is LSE (often not enabled).
 */

/*
 * @brief STM32WBA RNG configuration.
 */
typedef struct whal_Stm32wba_Rng_Cfg {
    whal_Timeout *timeout;     /* Optional timeout for poll loops */
} whal_Stm32wba_Rng_Cfg;

/*
 * @brief Platform-owned RNG device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32WBA_RNG_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Rng whal_Stm32wba_Rng_Dev;

/*
 * @brief Driver instance for the STM32WBA hardware RNG.
 */
extern const whal_RngDriver whal_Stm32wba_Rng_Driver;

/*
 * @brief Initialize the RNG (CONDRST sequence + enable RNGEN). The board
 *        must have selected a kernel clock source via RCC_CCIPR2.RNGSEL
 *        before calling this.
 *
 * @param rngDev RNG device instance.
 *
 * @retval WHAL_SUCCESS   RNG is ready.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ETIMEOUT  CONDRST did not complete within the configured timeout.
 * @retval WHAL_EHARDWARE Health check or seed error reported.
 */
whal_Error whal_Stm32wba_Rng_Init(whal_Rng *rngDev);

/*
 * @brief Deinitialize the RNG (clears RNGEN).
 *
 * @param rngDev RNG device instance.
 *
 * @retval WHAL_SUCCESS RNG disabled.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Stm32wba_Rng_Deinit(whal_Rng *rngDev);

/*
 * @brief Generate `rngDataSz` random bytes into `rngData`. Polls DRDY for
 *        each 32-bit sample.
 *
 * @param rngDev    RNG device instance.
 * @param rngData   Destination buffer.
 * @param rngDataSz Number of bytes to generate.
 *
 * @retval WHAL_SUCCESS   Bytes generated.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ETIMEOUT  DRDY did not assert within the configured timeout.
 * @retval WHAL_EHARDWARE Seed or clock error reported in RNG_SR.
 */
whal_Error whal_Stm32wba_Rng_Generate(whal_Rng *rngDev, void *rngData, size_t rngDataSz);

#endif /* WHAL_STM32WBA_RNG_H */
