/* stm32f0_wwdg.c
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

#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32F0_WWDG_DEV initializer */
#include <wolfHAL/watchdog/stm32f0_wwdg.h>
#include <wolfHAL/watchdog/watchdog.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

const whal_Watchdog whal_Stm32f0_Wwdg_Dev = WHAL_CFG_STM32F0_WWDG_DEV;

#define CR_REG 0x00
#define CR_T_Pos 0
#define CR_T_Msk (WHAL_BITMASK(7) << CR_T_Pos)
#define CR_WDGA_Pos 7
#define CR_WDGA_Msk (1UL << CR_WDGA_Pos)

#define CFR_REG 0x04
#define CFR_W_Pos 0
#define CFR_W_Msk (WHAL_BITMASK(7) << CFR_W_Pos)
#define CFR_WDGTB_Pos 7
#define CFR_WDGTB_Msk (WHAL_BITMASK(2) << CFR_WDGTB_Pos)
#define CFR_EWI_Pos 9
#define CFR_EWI_Msk (1UL << CFR_EWI_Pos)

#ifdef WHAL_CFG_STM32F0_WWDG_DIRECT_API_MAPPING
#define whal_Stm32f0_Wwdg_Init    whal_Watchdog_Init
#define whal_Stm32f0_Wwdg_Deinit  whal_Watchdog_Deinit
#define whal_Stm32f0_Wwdg_Refresh whal_Watchdog_Refresh
#endif

whal_Error whal_Stm32f0_Wwdg_Init(whal_Watchdog *wdgDev)
{
    const whal_Stm32f0_Wwdg_Cfg *cfg =
        (const whal_Stm32f0_Wwdg_Cfg *)whal_Stm32f0_Wwdg_Dev.cfg;
    size_t base = whal_Stm32f0_Wwdg_Dev.base;
    (void)wdgDev;

    if (cfg->prescaler > 3 || cfg->window > 0x7F || cfg->counter > 0x7F)
        return WHAL_EINVAL;

    whal_Reg_Update(base, CFR_REG, CFR_W_Msk | CFR_WDGTB_Msk,
                    whal_SetBits(CFR_W_Msk, CFR_W_Pos, cfg->window) |
                    whal_SetBits(CFR_WDGTB_Msk, CFR_WDGTB_Pos, cfg->prescaler));

    whal_Reg_Write(base, CR_REG, CR_WDGA_Msk | cfg->counter);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f0_Wwdg_Deinit(whal_Watchdog *wdgDev)
{
    (void)wdgDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f0_Wwdg_Refresh(whal_Watchdog *wdgDev)
{
    const whal_Stm32f0_Wwdg_Cfg *cfg =
        (const whal_Stm32f0_Wwdg_Cfg *)whal_Stm32f0_Wwdg_Dev.cfg;
    size_t base = whal_Stm32f0_Wwdg_Dev.base;
    (void)wdgDev;

    whal_Reg_Write(base, CR_REG, CR_WDGA_Msk | cfg->counter);

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32F0_WWDG_DIRECT_API_MAPPING
const whal_WatchdogDriver whal_Stm32f0_Wwdg_Driver = {
    .Init = whal_Stm32f0_Wwdg_Init,
    .Deinit = whal_Stm32f0_Wwdg_Deinit,
    .Refresh = whal_Stm32f0_Wwdg_Refresh,
};
#endif
