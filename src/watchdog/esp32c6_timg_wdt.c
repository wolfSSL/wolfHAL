/* esp32c6_timg_wdt.c
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

#include <wolfHAL/watchdog/esp32c6_timg_wdt.h>
#include <wolfHAL/watchdog/watchdog.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

#ifdef WHAL_CFG_ESP32C6_TIMG_WDT_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides WHAL_CFG_ESP32C6_TIMG_WDT_DEV initializer */
#endif

/* TIMG watchdog registers (TRM chapter 14), relative to the timer group
 * base. Writes are ignored unless WDTWPROTECT holds the write key, and
 * WDTCONFIG0 changes take effect only after CONF_UPDATE_EN is written. */

#define TIMG_WDTCONFIG0_REG             0x0048
#define TIMG_WDT_FLASHBOOT_MOD_EN_Pos   14
#define TIMG_WDT_FLASHBOOT_MOD_EN_Msk   (1UL << TIMG_WDT_FLASHBOOT_MOD_EN_Pos)
#define TIMG_WDT_CONF_UPDATE_EN_Pos     22
#define TIMG_WDT_CONF_UPDATE_EN_Msk     (1UL << TIMG_WDT_CONF_UPDATE_EN_Pos)
#define TIMG_WDT_STG3_Pos               23
#define TIMG_WDT_STG3_Msk               (WHAL_BITMASK(2) << TIMG_WDT_STG3_Pos)
#define TIMG_WDT_STG2_Pos               25
#define TIMG_WDT_STG2_Msk               (WHAL_BITMASK(2) << TIMG_WDT_STG2_Pos)
#define TIMG_WDT_STG1_Pos               27
#define TIMG_WDT_STG1_Msk               (WHAL_BITMASK(2) << TIMG_WDT_STG1_Pos)
#define TIMG_WDT_STG0_Pos               29
#define TIMG_WDT_STG0_Msk               (WHAL_BITMASK(2) << TIMG_WDT_STG0_Pos)
#define TIMG_WDT_EN_Pos                 31
#define TIMG_WDT_EN_Msk                 (1UL << TIMG_WDT_EN_Pos)

/* Stage timeout action: 0 = off, 3 = system reset. */
#define TIMG_WDT_STG_OFF                0
#define TIMG_WDT_STG_SYS_RESET          3

#define TIMG_WDTCONFIG1_REG             0x004C
#define TIMG_WDT_DIVCNT_RST_Pos         0
#define TIMG_WDT_DIVCNT_RST_Msk         (1UL << TIMG_WDT_DIVCNT_RST_Pos)
#define TIMG_WDT_CLK_PRESCALE_Pos       16
#define TIMG_WDT_CLK_PRESCALE_Msk       (WHAL_BITMASK(16) << TIMG_WDT_CLK_PRESCALE_Pos)

#define TIMG_WDTCONFIG2_REG             0x0050
#define TIMG_WDTFEED_REG                0x0060
#define TIMG_WDTWPROTECT_REG            0x0064

#define TIMG_WDT_WKEY                   0x50D83AA1UL

#ifdef WHAL_CFG_ESP32C6_TIMG_WDT_DIRECT_API_MAPPING
#define whal_Esp32c6_TimgWdt_Init    whal_Watchdog_Init
#define whal_Esp32c6_TimgWdt_Deinit  whal_Watchdog_Deinit
#define whal_Esp32c6_TimgWdt_Refresh whal_Watchdog_Refresh
#endif /* WHAL_CFG_ESP32C6_TIMG_WDT_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_ESP32C6_TIMG_WDT_SINGLE_INSTANCE
const whal_Watchdog whal_Esp32c6_TimgWdt_Dev = WHAL_CFG_ESP32C6_TIMG_WDT_DEV;
#endif

whal_Error whal_Esp32c6_TimgWdt_Init(whal_Watchdog *wdgDev)
{
#ifdef WHAL_CFG_ESP32C6_TIMG_WDT_SINGLE_INSTANCE
    const whal_Esp32c6_TimgWdt_Cfg *cfg =
        (const whal_Esp32c6_TimgWdt_Cfg *)whal_Esp32c6_TimgWdt_Dev.cfg;
    size_t base = whal_Esp32c6_TimgWdt_Dev.base;
    (void)wdgDev;
#else
    const whal_Esp32c6_TimgWdt_Cfg *cfg;
    size_t base;

    if (!wdgDev || !wdgDev->cfg) {
        return WHAL_EINVAL;
    }

    base = wdgDev->base;
    cfg = (const whal_Esp32c6_TimgWdt_Cfg *)wdgDev->cfg;
#endif

    if (cfg->prescale == 0) {
        return WHAL_ENOTSUP;
    }

    whal_Reg_Write(base, TIMG_WDTWPROTECT_REG, TIMG_WDT_WKEY);

    whal_Reg_Write(base, TIMG_WDTCONFIG1_REG,
                   whal_SetBits(TIMG_WDT_CLK_PRESCALE_Msk,
                                TIMG_WDT_CLK_PRESCALE_Pos, cfg->prescale) |
                   TIMG_WDT_DIVCNT_RST_Msk);
    whal_Reg_Write(base, TIMG_WDTCONFIG2_REG, cfg->stg0Hold);
    whal_Reg_Update(base, TIMG_WDTCONFIG0_REG,
                    TIMG_WDT_EN_Msk | TIMG_WDT_STG0_Msk | TIMG_WDT_STG1_Msk |
                    TIMG_WDT_STG2_Msk | TIMG_WDT_STG3_Msk |
                    TIMG_WDT_FLASHBOOT_MOD_EN_Msk,
                    TIMG_WDT_EN_Msk |
                    whal_SetBits(TIMG_WDT_STG0_Msk, TIMG_WDT_STG0_Pos,
                                 TIMG_WDT_STG_SYS_RESET) |
                    whal_SetBits(TIMG_WDT_STG1_Msk, TIMG_WDT_STG1_Pos,
                                 TIMG_WDT_STG_OFF) |
                    whal_SetBits(TIMG_WDT_STG2_Msk, TIMG_WDT_STG2_Pos,
                                 TIMG_WDT_STG_OFF) |
                    whal_SetBits(TIMG_WDT_STG3_Msk, TIMG_WDT_STG3_Pos,
                                 TIMG_WDT_STG_OFF));
    whal_Reg_Update(base, TIMG_WDTCONFIG0_REG, TIMG_WDT_CONF_UPDATE_EN_Msk,
                    TIMG_WDT_CONF_UPDATE_EN_Msk);
    whal_Reg_Write(base, TIMG_WDTFEED_REG, 1);

    whal_Reg_Write(base, TIMG_WDTWPROTECT_REG, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_TimgWdt_Deinit(whal_Watchdog *wdgDev)
{
#ifdef WHAL_CFG_ESP32C6_TIMG_WDT_SINGLE_INSTANCE
    size_t base = whal_Esp32c6_TimgWdt_Dev.base;
    (void)wdgDev;
#else
    size_t base;

    if (!wdgDev) {
        return WHAL_EINVAL;
    }

    base = wdgDev->base;
#endif

    whal_Reg_Write(base, TIMG_WDTWPROTECT_REG, TIMG_WDT_WKEY);
    whal_Reg_Update(base, TIMG_WDTCONFIG0_REG,
                    TIMG_WDT_EN_Msk | TIMG_WDT_FLASHBOOT_MOD_EN_Msk, 0);
    whal_Reg_Update(base, TIMG_WDTCONFIG0_REG, TIMG_WDT_CONF_UPDATE_EN_Msk,
                    TIMG_WDT_CONF_UPDATE_EN_Msk);
    whal_Reg_Write(base, TIMG_WDTWPROTECT_REG, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_TimgWdt_Refresh(whal_Watchdog *wdgDev)
{
#ifdef WHAL_CFG_ESP32C6_TIMG_WDT_SINGLE_INSTANCE
    size_t base = whal_Esp32c6_TimgWdt_Dev.base;
    (void)wdgDev;
#else
    size_t base;

    if (!wdgDev) {
        return WHAL_EINVAL;
    }

    base = wdgDev->base;
#endif

    whal_Reg_Write(base, TIMG_WDTWPROTECT_REG, TIMG_WDT_WKEY);
    whal_Reg_Write(base, TIMG_WDTFEED_REG, 1);
    whal_Reg_Write(base, TIMG_WDTWPROTECT_REG, 0);

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_ESP32C6_TIMG_WDT_DIRECT_API_MAPPING
const whal_WatchdogDriver whal_Esp32c6_TimgWdt_Driver = {
    .Init = whal_Esp32c6_TimgWdt_Init,
    .Deinit = whal_Esp32c6_TimgWdt_Deinit,
    .Refresh = whal_Esp32c6_TimgWdt_Refresh,
};
#endif /* !WHAL_CFG_ESP32C6_TIMG_WDT_DIRECT_API_MAPPING */
