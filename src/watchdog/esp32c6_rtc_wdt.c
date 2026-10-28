/* esp32c6_rtc_wdt.c
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

#include "wolfHAL_board.h"  /* provides WHAL_CFG_ESP32C6_RTC_WDT_DEV initializer */
#include <wolfHAL/watchdog/esp32c6_rtc_wdt.h>
#include <wolfHAL/watchdog/watchdog.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

const whal_Watchdog whal_Esp32c6_RtcWdt_Dev = WHAL_CFG_ESP32C6_RTC_WDT_DEV;

/* RTC_WDT register offsets and fields (TRM chapter 15). Every RWDT register
 * write is ignored unless WPROTECT holds the write key; the SWD has its own
 * write-protect register with the same key. */

#define RTC_WDT_CONFIG0_REG             0x0000
#define RTC_WDT_FLASHBOOT_MOD_EN_Pos    12
#define RTC_WDT_FLASHBOOT_MOD_EN_Msk    (1UL << RTC_WDT_FLASHBOOT_MOD_EN_Pos)
#define RTC_WDT_STG3_Pos                19
#define RTC_WDT_STG3_Msk                (WHAL_BITMASK(3) << RTC_WDT_STG3_Pos)
#define RTC_WDT_STG2_Pos                22
#define RTC_WDT_STG2_Msk                (WHAL_BITMASK(3) << RTC_WDT_STG2_Pos)
#define RTC_WDT_STG1_Pos                25
#define RTC_WDT_STG1_Msk                (WHAL_BITMASK(3) << RTC_WDT_STG1_Pos)
#define RTC_WDT_STG0_Pos                28
#define RTC_WDT_STG0_Msk                (WHAL_BITMASK(3) << RTC_WDT_STG0_Pos)
#define RTC_WDT_EN_Pos                  31
#define RTC_WDT_EN_Msk                  (1UL << RTC_WDT_EN_Pos)

/* Stage timeout action: 0 = off, 4 = system reset. */
#define RTC_WDT_STG_OFF                 0
#define RTC_WDT_STG_SYS_RESET           4

#define RTC_WDT_CONFIG1_REG             0x0004

#define RTC_WDT_FEED_REG                0x0014
#define RTC_WDT_FEED_Pos                31
#define RTC_WDT_FEED_Msk                (1UL << RTC_WDT_FEED_Pos)

#define RTC_WDT_WPROTECT_REG            0x0018

#define RTC_WDT_SWD_CONFIG_REG          0x001C
#define RTC_WDT_SWD_DISABLE_Pos         30
#define RTC_WDT_SWD_DISABLE_Msk         (1UL << RTC_WDT_SWD_DISABLE_Pos)

#define RTC_WDT_SWD_WPROTECT_REG        0x0020

#define RTC_WDT_WKEY                    0x50D83AA1UL

#ifdef WHAL_CFG_ESP32C6_RTC_WDT_DIRECT_API_MAPPING
#define whal_Esp32c6_RtcWdt_Init    whal_Watchdog_Init
#define whal_Esp32c6_RtcWdt_Deinit  whal_Watchdog_Deinit
#define whal_Esp32c6_RtcWdt_Refresh whal_Watchdog_Refresh
#endif /* WHAL_CFG_ESP32C6_RTC_WDT_DIRECT_API_MAPPING */

whal_Error whal_Esp32c6_RtcWdt_Init(whal_Watchdog *wdgDev)
{
    const whal_Esp32c6_RtcWdt_Cfg *cfg =
        (const whal_Esp32c6_RtcWdt_Cfg *)whal_Esp32c6_RtcWdt_Dev.cfg;
    size_t base = whal_Esp32c6_RtcWdt_Dev.base;
    (void)wdgDev;

    whal_Reg_Write(base, RTC_WDT_WPROTECT_REG, RTC_WDT_WKEY);

    whal_Reg_Write(base, RTC_WDT_CONFIG1_REG, cfg->stg0Hold);
    whal_Reg_Update(base, RTC_WDT_CONFIG0_REG,
                    RTC_WDT_EN_Msk | RTC_WDT_STG0_Msk | RTC_WDT_STG1_Msk |
                    RTC_WDT_STG2_Msk | RTC_WDT_STG3_Msk |
                    RTC_WDT_FLASHBOOT_MOD_EN_Msk,
                    RTC_WDT_EN_Msk |
                    whal_SetBits(RTC_WDT_STG0_Msk, RTC_WDT_STG0_Pos,
                                 RTC_WDT_STG_SYS_RESET) |
                    whal_SetBits(RTC_WDT_STG1_Msk, RTC_WDT_STG1_Pos,
                                 RTC_WDT_STG_OFF) |
                    whal_SetBits(RTC_WDT_STG2_Msk, RTC_WDT_STG2_Pos,
                                 RTC_WDT_STG_OFF) |
                    whal_SetBits(RTC_WDT_STG3_Msk, RTC_WDT_STG3_Pos,
                                 RTC_WDT_STG_OFF));
    whal_Reg_Write(base, RTC_WDT_FEED_REG, RTC_WDT_FEED_Msk);

    whal_Reg_Write(base, RTC_WDT_WPROTECT_REG, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_RtcWdt_Deinit(whal_Watchdog *wdgDev)
{
    size_t base = whal_Esp32c6_RtcWdt_Dev.base;
    (void)wdgDev;

    whal_Reg_Write(base, RTC_WDT_WPROTECT_REG, RTC_WDT_WKEY);
    whal_Reg_Update(base, RTC_WDT_CONFIG0_REG,
                    RTC_WDT_EN_Msk | RTC_WDT_FLASHBOOT_MOD_EN_Msk, 0);
    whal_Reg_Write(base, RTC_WDT_WPROTECT_REG, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_RtcWdt_Refresh(whal_Watchdog *wdgDev)
{
    size_t base = whal_Esp32c6_RtcWdt_Dev.base;
    (void)wdgDev;

    whal_Reg_Write(base, RTC_WDT_WPROTECT_REG, RTC_WDT_WKEY);
    whal_Reg_Write(base, RTC_WDT_FEED_REG, RTC_WDT_FEED_Msk);
    whal_Reg_Write(base, RTC_WDT_WPROTECT_REG, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_RtcWdt_Ext_DisableSwd(whal_Watchdog *wdgDev)
{
    size_t base = whal_Esp32c6_RtcWdt_Dev.base;
    (void)wdgDev;

    whal_Reg_Write(base, RTC_WDT_SWD_WPROTECT_REG, RTC_WDT_WKEY);
    whal_Reg_Update(base, RTC_WDT_SWD_CONFIG_REG, RTC_WDT_SWD_DISABLE_Msk,
                    RTC_WDT_SWD_DISABLE_Msk);
    whal_Reg_Write(base, RTC_WDT_SWD_WPROTECT_REG, 0);

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_ESP32C6_RTC_WDT_DIRECT_API_MAPPING
const whal_WatchdogDriver whal_Esp32c6_RtcWdt_Driver = {
    .Init = whal_Esp32c6_RtcWdt_Init,
    .Deinit = whal_Esp32c6_RtcWdt_Deinit,
    .Refresh = whal_Esp32c6_RtcWdt_Refresh,
};
#endif /* !WHAL_CFG_ESP32C6_RTC_WDT_DIRECT_API_MAPPING */
