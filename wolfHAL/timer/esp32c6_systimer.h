/* esp32c6_systimer.h
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

#ifndef WHAL_ESP32C6_SYSTIMER_H
#define WHAL_ESP32C6_SYSTIMER_H

#include <stddef.h>
#include <stdint.h>
#include <wolfHAL/timer/timer.h>

/*
 * @file esp32c6_systimer.h
 * @brief ESP32-C6 System Timer (SYSTIMER) driver configuration.
 *
 * The ESP32-C6 SYSTIMER has two 52-bit counters (UNIT0/1) and three comparators
 * (COMP0/1/2). Each whal_Timer instance drives one comparator watching one
 * counter; multiple instances may share a counter as a common time base.
 */

/*
 * @brief Alarm mode for the comparator.
 */
typedef enum {
    WHAL_ESP32C6_SYSTIMER_PERIODIC, /* Fire every `period` ticks (period mode). */
    WHAL_ESP32C6_SYSTIMER_ONESHOT,  /* Fire once at absolute `compValue` (target mode). */
} whal_Esp32c6_Systimer_Mode;

/*
 * @brief Counter unit (52-bit time base) the comparator watches.
 */
typedef enum {
    WHAL_ESP32C6_SYSTIMER_UNIT0,
    WHAL_ESP32C6_SYSTIMER_UNIT1,
} whal_Esp32c6_Systimer_Unit;

/*
 * @brief Comparator (alarm channel).
 */
typedef enum {
    WHAL_ESP32C6_SYSTIMER_COMP0,
    WHAL_ESP32C6_SYSTIMER_COMP1,
    WHAL_ESP32C6_SYSTIMER_COMP2,
} whal_Esp32c6_Systimer_Comp;

/*
 * @brief SYSTIMER configuration: one comparator (comp) watching one counter
 * (unit). Periodic mode uses `period`; one-shot mode uses `compValue`.
 */
typedef struct {
    whal_Esp32c6_Systimer_Mode mode;
    whal_Esp32c6_Systimer_Unit unit;
    whal_Esp32c6_Systimer_Comp comp;
    uint64_t compValue;  /* One-shot: absolute 52-bit alarm value. */
    uint32_t period;     /* Periodic: 26-bit alarm interval in ticks. */
} whal_Esp32c6_Systimer_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_ESP32C6_SYSTIMER_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_ESP32C6_SYSTIMER_SINGLE_INSTANCE
extern const whal_Timer whal_Esp32c6_Systimer_Dev;
#endif

#ifndef WHAL_CFG_ESP32C6_SYSTIMER_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the ESP32-C6 System Timer.
 */
extern const whal_TimerDriver whal_Esp32c6_Systimer_Driver;

/*
 * @brief Configure the comparator in periodic or one-shot mode and enable its
 * interrupt.
 *
 * @param timerDev Timer device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Esp32c6_Systimer_Init(whal_Timer *timerDev);

/*
 * @brief Disable the comparator and its interrupt. The shared counter unit
 * keeps running.
 *
 * @param timerDev Timer device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Esp32c6_Systimer_Deinit(whal_Timer *timerDev);

/*
 * @brief Start the counter unit and enable the comparator.
 *
 * @param timerDev Timer device instance.
 *
 * @retval WHAL_SUCCESS Timer started.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Esp32c6_Systimer_Start(whal_Timer *timerDev);

/*
 * @brief Disable the comparator. The shared counter unit keeps running.
 *
 * @param timerDev Timer device instance.
 *
 * @retval WHAL_SUCCESS Timer stopped.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Esp32c6_Systimer_Stop(whal_Timer *timerDev);

/*
 * @brief Reload the shared counter unit to 0. Affects every comparator on
 * that unit.
 *
 * @param timerDev Timer device instance.
 *
 * @retval WHAL_SUCCESS Timer reset.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Esp32c6_Systimer_Reset(whal_Timer *timerDev);
#endif /* !WHAL_CFG_ESP32C6_SYSTIMER_DIRECT_API_MAPPING */

#endif /* WHAL_ESP32C6_SYSTIMER_H */
