/* esp32c6_timg_wdt.h
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

#ifndef WHAL_ESP32C6_TIMG_WDT_H
#define WHAL_ESP32C6_TIMG_WDT_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/watchdog/watchdog.h>

/*
 * @file esp32c6_timg_wdt.h
 * @brief ESP32-C6 timer group watchdog (MWDT) driver.
 *
 * Each timer group (TIMG0, TIMG1) contains one MWDT; this driver owns only
 * the watchdog registers of the group. The ROM arms TIMG0's MWDT during flash
 * boot, so a Direct Boot image disables it early in Board_Init. The MWDT
 * clock (XTAL_CLK by default) is selected in PCR; the driver divides it by
 * prescale, and Init arms stage 0 to reset the system after stg0Hold ticks of
 * the divided clock.
 */

/*
 * @brief MWDT device configuration.
 */
typedef struct {
    uint16_t prescale; /* MWDT clock divider, 1-65535 */
    uint32_t stg0Hold; /* Stage 0 timeout, in divided MWDT clock ticks */
} whal_Esp32c6_TimgWdt_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_ESP32C6_TIMG_WDT_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_ESP32C6_TIMG_WDT_SINGLE_INSTANCE
extern const whal_Watchdog whal_Esp32c6_TimgWdt_Dev;
#endif

#ifndef WHAL_CFG_ESP32C6_TIMG_WDT_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the ESP32-C6 MWDT.
 */
extern const whal_WatchdogDriver whal_Esp32c6_TimgWdt_Driver;

/*
 * @brief Arm the MWDT to reset the system after stg0Hold ticks.
 *
 * Clears flash boot protection, programs the prescaler and stage 0 timeout,
 * sets stage 0 to system reset with stages 1-3 off, enables the MWDT and
 * feeds it.
 *
 * @param wdgDev Watchdog device instance.
 *
 * @retval WHAL_SUCCESS Watchdog started.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP prescale is 0.
 */
whal_Error whal_Esp32c6_TimgWdt_Init(whal_Watchdog *wdgDev);

/*
 * @brief Disable the MWDT, including flash boot protection.
 *
 * @param wdgDev Watchdog device instance.
 *
 * @retval WHAL_SUCCESS Watchdog disabled.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Esp32c6_TimgWdt_Deinit(whal_Watchdog *wdgDev);

/*
 * @brief Feed the MWDT.
 *
 * @param wdgDev Watchdog device instance.
 *
 * @retval WHAL_SUCCESS Watchdog fed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Esp32c6_TimgWdt_Refresh(whal_Watchdog *wdgDev);
#endif /* !WHAL_CFG_ESP32C6_TIMG_WDT_DIRECT_API_MAPPING */

#endif /* WHAL_ESP32C6_TIMG_WDT_H */
