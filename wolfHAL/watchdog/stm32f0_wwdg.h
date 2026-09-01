/* stm32f0_wwdg.h
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

#ifndef WHAL_STM32F0_WWDG_H
#define WHAL_STM32F0_WWDG_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/watchdog/watchdog.h>

/*
 * @file stm32f0_wwdg.h
 * @brief STM32F0 window watchdog (WWDG) driver.
 *
 * The F0 WWDG has a 2-bit WDGTB prescaler at CFR bits 8:7 (divides by
 * 1/2/4/8), unlike the WB which has a 3-bit WDGTB at bits 13:11.
 */

/*
 * @brief STM32F0 WWDG configuration.
 */
typedef struct {
    uint8_t prescaler; /* WDGTB value (0–3): div 1, 2, 4, or 8 */
    uint8_t window;    /* WWDG window value (0x40–0x7F) */
    uint8_t counter;   /* Initial counter value (0x40–0x7F) */
} whal_Stm32f0_Wwdg_Cfg;

/*
 * @brief Platform-owned WWDG device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32F0_WWDG_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Watchdog whal_Stm32f0_Wwdg_Dev;

#ifndef WHAL_CFG_STM32F0_WWDG_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the STM32F0 window watchdog.
 */
extern const whal_WatchdogDriver whal_Stm32f0_Wwdg_Driver;

/*
 * @brief Initialize and start the WWDG. Programs CFR (window/prescaler)
 *        and CR (enable + initial counter). The watchdog cannot be stopped
 *        once started.
 *
 * @param wdgDev Watchdog device instance.
 *
 * @retval WHAL_SUCCESS Watchdog started.
 * @retval WHAL_EINVAL  Null pointer or missing cfg.
 */
whal_Error whal_Stm32f0_Wwdg_Init(whal_Watchdog *wdgDev);

/*
 * @brief Deinitialize the WWDG (driver-side cleanup only — the hardware
 *        cannot be stopped).
 *
 * @param wdgDev Watchdog device instance.
 *
 * @retval WHAL_SUCCESS Driver deinitialized.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Stm32f0_Wwdg_Deinit(whal_Watchdog *wdgDev);

/*
 * @brief Refresh the WWDG counter. Must be called after the window opens
 *        and before the counter underflows, or a reset is generated.
 *
 * @param wdgDev Watchdog device instance.
 *
 * @retval WHAL_SUCCESS Counter refreshed.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Stm32f0_Wwdg_Refresh(whal_Watchdog *wdgDev);
#endif /* !WHAL_CFG_STM32F0_WWDG_DIRECT_API_MAPPING */

#endif /* WHAL_STM32F0_WWDG_H */
