/* systick.c
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

#include "wolfHAL_board.h"  /* provides WHAL_CFG_SYSTICK_DEV initializer */
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timer/timer.h>
#include <wolfHAL/timer/systick.h>

const whal_Timer whal_SysTick_Dev = WHAL_CFG_SYSTICK_DEV;

#define SYSTICK_CSR_REG 0x00
#define SYSTICK_CSR_ENABLE_Pos 0
#define SYSTICK_CSR_ENABLE_Msk (1UL << SYSTICK_CSR_ENABLE_Pos)

#define SYSTICK_CSR_TICKINT_Pos 1
#define SYSTICK_CSR_TICKINT_Msk (1UL << SYSTICK_CSR_TICKINT_Pos)

#define SYSTICK_CSR_CLKSOURCE_Pos 2
#define SYSTICK_CSR_CLKSOURCE_Msk (1UL << SYSTICK_CSR_CLKSOURCE_Pos)

#define SYSTICK_CSR_COUNTFLAG_Pos 16
#define SYSTICK_CSR_COUNTFLAG_Msk (1UL << SYSTICK_CSR_COUNTFLAG_Pos)

#define SYSTICK_RVR_REG 0x04
#define SYSTICK_RVR_RELOAD_Pos 0
#define SYSTICK_RVR_RELOAD_Msk (WHAL_BITMASK(24) << SYSTICK_RVR_RELOAD_Pos)

#ifdef WHAL_CFG_SYSTICK_TIMER_DIRECT_API_MAPPING
#define whal_SysTick_Init   whal_Timer_Init
#define whal_SysTick_Deinit whal_Timer_Deinit
#define whal_SysTick_Start  whal_Timer_Start
#define whal_SysTick_Stop   whal_Timer_Stop
#define whal_SysTick_Reset  whal_Timer_Reset
#endif /* WHAL_CFG_SYSTICK_TIMER_DIRECT_API_MAPPING */

whal_Error whal_SysTick_Init(whal_Timer *timerDev)
{
    const whal_SysTick_Cfg *cfg =
        (const whal_SysTick_Cfg *)whal_SysTick_Dev.cfg;
    size_t base = whal_SysTick_Dev.base;
    (void)timerDev;

    whal_Reg_Update(base, SYSTICK_CSR_REG,
                          SYSTICK_CSR_CLKSOURCE_Msk | SYSTICK_CSR_TICKINT_Msk,
                          whal_SetBits(SYSTICK_CSR_CLKSOURCE_Msk, SYSTICK_CSR_CLKSOURCE_Pos, cfg->clkSrc) |
                          whal_SetBits(SYSTICK_CSR_TICKINT_Msk, SYSTICK_CSR_TICKINT_Pos, cfg->tickInt));

    whal_Reg_Update(base, SYSTICK_RVR_REG,
                    SYSTICK_RVR_RELOAD_Msk,
                    whal_SetBits(SYSTICK_RVR_RELOAD_Msk, SYSTICK_RVR_RELOAD_Pos, cfg->cyclesPerTick));

    return WHAL_SUCCESS;
}

whal_Error whal_SysTick_Deinit(whal_Timer *timerDev)
{
    (void)timerDev;
    return WHAL_SUCCESS;
}

whal_Error whal_SysTick_Start(whal_Timer *timerDev)
{
    size_t base = whal_SysTick_Dev.base;
    (void)timerDev;

    whal_Reg_Update(base, SYSTICK_CSR_REG, SYSTICK_CSR_ENABLE_Msk,
                    whal_SetBits(SYSTICK_CSR_ENABLE_Msk, SYSTICK_CSR_ENABLE_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_SysTick_Stop(whal_Timer *timerDev)
{
    size_t base = whal_SysTick_Dev.base;
    (void)timerDev;

    whal_Reg_Update(base, SYSTICK_CSR_REG, SYSTICK_CSR_ENABLE_Msk,
                    whal_SetBits(SYSTICK_CSR_ENABLE_Msk, SYSTICK_CSR_ENABLE_Pos, 0));

    return WHAL_SUCCESS;
}

whal_Error whal_SysTick_Reset(whal_Timer *timerDev)
{
    (void)timerDev;
    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_SYSTICK_TIMER_DIRECT_API_MAPPING
const whal_TimerDriver whal_SysTick_Driver = {
    .Init = whal_SysTick_Init,
    .Deinit = whal_SysTick_Deinit,
    .Start = whal_SysTick_Start,
    .Stop = whal_SysTick_Stop,
    .Reset = whal_SysTick_Reset,
};
#endif /* !WHAL_CFG_SYSTICK_TIMER_DIRECT_API_MAPPING */
