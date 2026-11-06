/* timer.h
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

#ifndef WHAL_TIMER_H
#define WHAL_TIMER_H

#include <wolfHAL/error.h>
#include <stddef.h>
#include <stdint.h>

/*
 * @file timer.h
 * @brief Generic timer abstraction for periodic events or counters.
 */

typedef struct whal_Timer whal_Timer;

/*
 * @brief Driver vtable for timer devices.
 */
typedef struct {
    /* Initialize the timer hardware. */
    whal_Error (*Init)(whal_Timer *timerDev);
    /* Deinitialize the timer hardware. */
    whal_Error (*Deinit)(whal_Timer *timerDev);
    /* Start the timer running. */
    whal_Error (*Start)(whal_Timer *timerDev);
    /* Stop the timer. */
    whal_Error (*Stop)(whal_Timer *timerDev);
    /* Reset the timer counter and configuration as needed. */
    whal_Error (*Reset)(whal_Timer *timerDev);
} whal_TimerDriver;

/*
 * @brief Timer device instance pairing configuration with a driver.
 */
struct whal_Timer{
    const uintptr_t base;
    const whal_TimerDriver *driver;
    void *cfg;
};

/*
 * @brief Initialize a timer device.
 *
 * @param timerDev Timer instance to initialize.
 *
 * @retval WHAL_SUCCESS Driver-specific init completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Timer_Init(whal_Timer *timerDev);
/*
 * @brief Deinitialize a timer device.
 *
 * @param timerDev Timer instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Driver-specific deinit completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Timer_Deinit(whal_Timer *timerDev);
/*
 * @brief Start a timer.
 *
 * @param timerDev Timer instance to start.
 *
 * @retval WHAL_SUCCESS Timer started.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Timer_Start(whal_Timer *timerDev);
/*
 * @brief Stop a timer.
 *
 * @param timerDev Timer instance to stop.
 *
 * @retval WHAL_SUCCESS Timer stopped.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Timer_Stop(whal_Timer *timerDev);
/*
 * @brief Reset a timer.
 *
 * @param timerDev Timer instance to reset.
 *
 * @retval WHAL_SUCCESS Timer reset.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Timer_Reset(whal_Timer *timerDev);

#endif /* WHAL_TIMER_H */
