/* esp32c6_rtc_wdt.h
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

#ifndef WHAL_ESP32C6_RTC_WDT_H
#define WHAL_ESP32C6_RTC_WDT_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/watchdog/watchdog.h>

/*
 * @file esp32c6_rtc_wdt.h
 * @brief ESP32-C6 RTC watchdog (RWDT) driver.
 *
 * The RTC_WDT block holds the RTC watchdog and the analog super watchdog
 * (SWD). The ROM arms the RWDT during flash boot and the SWD must be fed
 * roughly once a second; a Direct Boot image has no second-stage bootloader
 * to handle either, so the board disables both early in Board_Init. Init arms
 * stage 0 to reset the system after stg0Hold ticks of the RTC slow clock.
 */

/*
 * @brief RWDT device configuration.
 */
typedef struct {
    uint32_t stg0Hold; /* Stage 0 timeout, in RTC slow clock ticks */
} whal_Esp32c6_RtcWdt_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_ESP32C6_RTC_WDT_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Watchdog whal_Esp32c6_RtcWdt_Dev;

#ifndef WHAL_CFG_ESP32C6_RTC_WDT_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the ESP32-C6 RWDT.
 */
extern const whal_WatchdogDriver whal_Esp32c6_RtcWdt_Driver;

/*
 * @brief Arm the RWDT to reset the system after stg0Hold ticks.
 *
 * Clears flash boot protection, sets stage 0 to system reset with stages 1-3
 * off, enables the RWDT and feeds it.
 *
 * @param wdgDev Watchdog device instance.
 *
 * @retval WHAL_SUCCESS Watchdog started.
 */
whal_Error whal_Esp32c6_RtcWdt_Init(whal_Watchdog *wdgDev);

/*
 * @brief Disable the RWDT, including flash boot protection.
 *
 * @param wdgDev Watchdog device instance.
 *
 * @retval WHAL_SUCCESS Watchdog disabled.
 */
whal_Error whal_Esp32c6_RtcWdt_Deinit(whal_Watchdog *wdgDev);

/*
 * @brief Feed the RWDT.
 *
 * @param wdgDev Watchdog device instance.
 *
 * @retval WHAL_SUCCESS Watchdog fed.
 */
whal_Error whal_Esp32c6_RtcWdt_Refresh(whal_Watchdog *wdgDev);
#endif /* !WHAL_CFG_ESP32C6_RTC_WDT_DIRECT_API_MAPPING */

/*
 * @brief Disable the super watchdog (SWD).
 *
 * @param wdgDev Watchdog device instance.
 *
 * @retval WHAL_SUCCESS SWD disabled.
 */
whal_Error whal_Esp32c6_RtcWdt_Ext_DisableSwd(whal_Watchdog *wdgDev);

#endif /* WHAL_ESP32C6_RTC_WDT_H */
