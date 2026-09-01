/* stm32u5_rng.h
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

#ifndef WHAL_STM32U5_RNG_H
#define WHAL_STM32U5_RNG_H

/**
 * @file stm32u5_rng.h
 * @brief STM32U5 RNG driver (alias for STM32WBA RNG).
 *
 * The STM32U5 RNG peripheral is register-compatible with the STM32WBA RNG
 * (CONDRST/CONFIGLOCK + NIST SP800-90B health check sequence). This header
 * re-exports the STM32WBA RNG driver types and symbols under STM32U5-specific
 * names.
 */

#include <wolfHAL/rng/stm32wba_rng.h>

typedef whal_Stm32wba_Rng_Cfg whal_Stm32u5_Rng_Cfg;

#define whal_Stm32u5_Rng_Dev whal_Stm32wba_Rng_Dev

#ifndef WHAL_CFG_STM32U5_RNG_DIRECT_API_MAPPING
#define whal_Stm32u5_Rng_Driver   whal_Stm32wba_Rng_Driver
#define whal_Stm32u5_Rng_Init     whal_Stm32wba_Rng_Init
#define whal_Stm32u5_Rng_Deinit   whal_Stm32wba_Rng_Deinit
#define whal_Stm32u5_Rng_Generate whal_Stm32wba_Rng_Generate
#endif /* !WHAL_CFG_STM32U5_RNG_DIRECT_API_MAPPING */

/* Config initializer macro alias. The U5 wolfHAL_board.h supplies the body
 * under the U5-prefixed name; the WBA driver source consumes the WBA name. */
#define WHAL_CFG_STM32WBA_RNG_DEV WHAL_CFG_STM32U5_RNG_DEV

#endif /* WHAL_STM32U5_RNG_H */
