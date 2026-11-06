/* watchdog.h
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

#ifndef WHAL_WATCHDOG_H
#define WHAL_WATCHDOG_H

#include <wolfHAL/error.h>
#include <stdint.h>
#include <stddef.h>

/*
 * @file watchdog.h
 * @brief Generic watchdog timer abstraction and driver interface.
 *
 * The watchdog timer resets the system if not refreshed within a
 * configured timeout period. Init configures and starts the watchdog.
 * On most hardware, once started the watchdog cannot be stopped —
 * only a system reset clears it.
 */

typedef struct whal_Watchdog whal_Watchdog;

/*
 * @brief Driver vtable for watchdog devices.
 */
typedef struct {
    /* Configure and start the watchdog. */
    whal_Error (*Init)(whal_Watchdog *wdgDev);
    /* Deinitialize the watchdog (may be unsupported by hardware). */
    whal_Error (*Deinit)(whal_Watchdog *wdgDev);
    /* Refresh the watchdog counter to prevent reset. */
    whal_Error (*Refresh)(whal_Watchdog *wdgDev);
} whal_WatchdogDriver;

/*
 * @brief Watchdog device instance with a base address, driver vtable, and platform-specific cfg.
 */
struct whal_Watchdog {
    const uintptr_t base;
    const whal_WatchdogDriver *driver;
    void *cfg;
};

/*
 * @brief Configure and start the watchdog.
 *
 * Configures the watchdog timeout from the platform-specific cfg
 * and starts the countdown. On most hardware, the watchdog cannot
 * be stopped after this call.
 *
 * @param wdgDev Pointer to the watchdog instance.
 *
 * @retval WHAL_SUCCESS Watchdog started.
 * @retval WHAL_EINVAL  Null pointer or invalid configuration.
 */
whal_Error whal_Watchdog_Init(whal_Watchdog *wdgDev);
/*
 * @brief Deinitialize the watchdog.
 *
 * On hardware that does not support stopping the watchdog (e.g.
 * STM32 IWDG), this function has no effect.
 *
 * @param wdgDev Pointer to the watchdog instance.
 *
 * @retval WHAL_SUCCESS Watchdog stopped or no-op completed.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Watchdog_Deinit(whal_Watchdog *wdgDev);
/*
 * @brief Refresh the watchdog counter to prevent reset.
 *
 * Must be called periodically within the configured timeout
 * window. Failure to call this results in a system reset.
 *
 * @param wdgDev Pointer to the watchdog instance.
 *
 * @retval WHAL_SUCCESS Counter refreshed.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Watchdog_Refresh(whal_Watchdog *wdgDev);

#endif /* WHAL_WATCHDOG_H */
