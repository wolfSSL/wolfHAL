/* stm32wb_iwdg.c
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

#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32WB_IWDG_DEV initializer */
#include <wolfHAL/watchdog/stm32wb_iwdg.h>
#include <wolfHAL/watchdog/watchdog.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>

const whal_Watchdog whal_Stm32wb_Iwdg_Dev = WHAL_CFG_STM32WB_IWDG_DEV;

/*
 * STM32WB IWDG Register Definitions
 *
 * The IWDG is a free-running downcounter clocked from the LSI (~32 kHz).
 * Once started via the key register, it cannot be stopped. If the counter
 * reaches zero without being refreshed, a system reset is generated.
 */

/* Key Register — write-only, controls access and refresh */
#define IWDG_KR_REG     0x00
#define IWDG_KEY_START  0xCCCC  /* Start the watchdog */
#define IWDG_KEY_RELOAD 0xAAAA  /* Reload the counter */
#define IWDG_KEY_ACCESS 0x5555  /* Enable write access to PR, RLR, WINR */

/* Prescaler Register */
#define IWDG_PR_REG     0x04
#define IWDG_PR_Pos     0
#define IWDG_PR_Msk     (0x7UL << IWDG_PR_Pos)

/* Reload Register */
#define IWDG_RLR_REG    0x08
#define IWDG_RLR_Pos    0
#define IWDG_RLR_Msk    (0xFFFUL << IWDG_RLR_Pos)

/* Status Register */
#define IWDG_SR_REG     0x0C
#define IWDG_SR_PVU_Pos 0       /* Prescaler value update */
#define IWDG_SR_PVU_Msk (1UL << IWDG_SR_PVU_Pos)
#define IWDG_SR_RVU_Pos 1       /* Reload value update */
#define IWDG_SR_RVU_Msk (1UL << IWDG_SR_RVU_Pos)

#ifdef WHAL_CFG_STM32WB_IWDG_DIRECT_API_MAPPING
#define whal_Stm32wb_Iwdg_Init    whal_Watchdog_Init
#define whal_Stm32wb_Iwdg_Deinit  whal_Watchdog_Deinit
#define whal_Stm32wb_Iwdg_Refresh whal_Watchdog_Refresh
#endif

whal_Error whal_Stm32wb_Iwdg_Init(whal_Watchdog *wdgDev)
{
    const whal_Stm32wb_Iwdg_Cfg *cfg =
        (const whal_Stm32wb_Iwdg_Cfg *)whal_Stm32wb_Iwdg_Dev.cfg;
    size_t base = whal_Stm32wb_Iwdg_Dev.base;
    whal_Error err;
    (void)wdgDev;

    if (cfg->prescaler > 6 || cfg->reload > 0xFFF) {
        return WHAL_EINVAL;
    }

    /* Start the IWDG */
    whal_Reg_Write(base, IWDG_KR_REG, IWDG_KEY_START);

    /* Enable register access */
    whal_Reg_Write(base, IWDG_KR_REG, IWDG_KEY_ACCESS);

    /* Set prescaler */
    whal_Reg_Write(base, IWDG_PR_REG, cfg->prescaler);

    /* Set reload value */
    whal_Reg_Write(base, IWDG_RLR_REG, cfg->reload);

    /* Wait for registers to update */
    err = whal_Reg_ReadPoll(base, IWDG_SR_REG,
                            IWDG_SR_PVU_Msk | IWDG_SR_RVU_Msk, 0,
                            cfg->timeout);
    if (err)
        return err;

    /* Refresh counter with new reload value */
    whal_Reg_Write(base, IWDG_KR_REG, IWDG_KEY_RELOAD);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Iwdg_Deinit(whal_Watchdog *wdgDev)
{
    (void)wdgDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Iwdg_Refresh(whal_Watchdog *wdgDev)
{
    (void)wdgDev;
    whal_Reg_Write(whal_Stm32wb_Iwdg_Dev.base, IWDG_KR_REG, IWDG_KEY_RELOAD);

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32WB_IWDG_DIRECT_API_MAPPING
const whal_WatchdogDriver whal_Stm32wb_Iwdg_Driver = {
    .Init = whal_Stm32wb_Iwdg_Init,
    .Deinit = whal_Stm32wb_Iwdg_Deinit,
    .Refresh = whal_Stm32wb_Iwdg_Refresh,
};
#endif
