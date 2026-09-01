/* stm32f3_wwdg.h
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

#ifndef WHAL_STM32F3_WWDG_H
#define WHAL_STM32F3_WWDG_H

/*
 * @file stm32f3_wwdg.h
 * @brief STM32F3 WWDG driver (alias for STM32F0 WWDG).
 *
 * The STM32F3 WWDG peripheral uses the same register layout as the STM32F0
 * (2-bit WDGTB prescaler at CFR bits 8:7).
 */

#include <wolfHAL/watchdog/stm32f0_wwdg.h>

typedef whal_Stm32f0_Wwdg_Cfg whal_Stm32f3_Wwdg_Cfg;

#define whal_Stm32f3_Wwdg_Dev whal_Stm32f0_Wwdg_Dev

#ifndef WHAL_CFG_STM32F3_WWDG_DIRECT_API_MAPPING
#define whal_Stm32f3_Wwdg_Driver  whal_Stm32f0_Wwdg_Driver
#define whal_Stm32f3_Wwdg_Init    whal_Stm32f0_Wwdg_Init
#define whal_Stm32f3_Wwdg_Deinit  whal_Stm32f0_Wwdg_Deinit
#define whal_Stm32f3_Wwdg_Refresh whal_Stm32f0_Wwdg_Refresh
#endif /* !WHAL_CFG_STM32F3_WWDG_DIRECT_API_MAPPING */

/* Config initializer macro alias. The F3 wolfHAL_board.h supplies the body
 * under the F3-prefixed name; the F0 driver source consumes the F0 name. */
#define WHAL_CFG_STM32F0_WWDG_DEV WHAL_CFG_STM32F3_WWDG_DEV

#endif /* WHAL_STM32F3_WWDG_H */
