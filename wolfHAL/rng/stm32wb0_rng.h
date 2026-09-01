/* stm32wb0_rng.h
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

#ifndef WHAL_STM32WB0_RNG_H
#define WHAL_STM32WB0_RNG_H

#include <stdint.h>
#include <wolfHAL/rng/rng.h>
#include <wolfHAL/timeout.h>

/**
 * @file stm32wb0_rng.h
 * @brief STM32WB0 RNG driver configuration.
 *
 * The STM32WB0 RNG is a Bluetooth-LE-oriented true random number generator
 * that differs from the standard STM32 RNG IP:
 *   - RNG_VAL is a 16-bit register (not 32-bit DR), delivered every 20
 *     RNGCLK cycles. Reads stall the AHB bus until a value is available.
 *   - Only 32-bit accesses are allowed on the RNG registers; an 8-bit or
 *     16-bit access generates an AHB fault.
 *   - RNG_CR has RNG_DIS at bit 2 (active-low disable; cleared at reset)
 *     and TST_CLK at bit 3.
 *   - RNG_SR exposes RNGRDY (bit 0), REVCLK (bit 1), and FAULT (bit 2).
 *   - No CONDRST, no CED, no clock source selection — RNGCLK is a fixed
 *     16 MHz internal feed enabled through RCC_AHBENR.RNGEN.
 */

/**
 * @brief RNG device configuration.
 */
typedef struct whal_Stm32wb0_Rng_Cfg {
    whal_Timeout *timeout;
} whal_Stm32wb0_Rng_Cfg;

/**
 * @brief Platform-owned RNG device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32WB0_RNG_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Rng whal_Stm32wb0_Rng_Dev;

#ifndef WHAL_CFG_STM32WB0_RNG_DIRECT_API_MAPPING
/**
 * @brief Driver instance for STM32WB0 RNG peripheral.
 */
extern const whal_RngDriver whal_Stm32wb0_Rng_Driver;

/**
 * @brief Initialize the STM32WB0 RNG peripheral.
 *
 * Enables the RNG analog block (clears RNG_DIS).
 *
 * @param rngDev RNG device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 */
whal_Error whal_Stm32wb0_Rng_Init(whal_Rng *rngDev);
/**
 * @brief Deinitialize the STM32WB0 RNG peripheral.
 *
 * Disables the RNG by setting RNG_DIS to put the internal free-running
 * oscillators into power-down.
 *
 * @param rngDev RNG device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 */
whal_Error whal_Stm32wb0_Rng_Deinit(whal_Rng *rngDev);
/**
 * @brief Generate random data.
 *
 * Reads 16-bit RNG_VAL values until rngDataSz bytes have been delivered.
 *
 * @param rngDev    RNG device instance.
 * @param rngData   Destination buffer.
 * @param rngDataSz Number of random bytes to generate.
 *
 * @retval WHAL_SUCCESS  Buffer filled with random data.
 * @retval WHAL_EINVAL   Null destination buffer.
 */
whal_Error whal_Stm32wb0_Rng_Generate(whal_Rng *rngDev, void *rngData,
                                      size_t rngDataSz);
#endif /* !WHAL_CFG_STM32WB0_RNG_DIRECT_API_MAPPING */

#endif /* WHAL_STM32WB0_RNG_H */
