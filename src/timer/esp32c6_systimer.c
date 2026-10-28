/* esp32c6_systimer.c
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

#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timer/timer.h>
#include <wolfHAL/timer/esp32c6_systimer.h>

#ifdef WHAL_CFG_ESP32C6_SYSTIMER_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides WHAL_CFG_ESP32C6_SYSTIMER_DEV initializer */
#endif

/* SYSTIMER register offsets and field positions (TRM chapter 13).
 * Offsets are relative to WHAL_ESP32C6_SYSTIMER_BASE (0x6000A000). The block has
 * two 52-bit counter units (UNIT0/1) and three 52-bit comparators (COMP0/1/2,
 * named TARGET0/1/2 in the register map); the u/t index selects the instance.
 * A 52-bit value is split across a high-20-bit HI register and a low-32-bit LO
 * register. Only fields with sub-register layout get _Pos/_Msk; LO registers
 * hold a plain 32-bit value. UNIT0/UNIT1 share a field layout, as do the three
 * comparators, so those field macros are defined once and reused. */

/* Global config: clock gate, per-unit enable/stall, per-comparator enable. */
#define SYSTIMER_CONF_REG                       0x0000
#define SYSTIMER_CLK_EN_Pos                     31
#define SYSTIMER_CLK_EN_Msk                     (1UL << SYSTIMER_CLK_EN_Pos)
#define SYSTIMER_TIMER_UNIT0_WORK_EN_Pos        30
#define SYSTIMER_TIMER_UNIT0_WORK_EN_Msk        (1UL << SYSTIMER_TIMER_UNIT0_WORK_EN_Pos)
#define SYSTIMER_TIMER_UNIT1_WORK_EN_Pos        29
#define SYSTIMER_TIMER_UNIT1_WORK_EN_Msk        (1UL << SYSTIMER_TIMER_UNIT1_WORK_EN_Pos)
#define SYSTIMER_TIMER_UNIT0_CORE0_STALL_EN_Pos 28
#define SYSTIMER_TIMER_UNIT0_CORE0_STALL_EN_Msk (1UL << SYSTIMER_TIMER_UNIT0_CORE0_STALL_EN_Pos)
#define SYSTIMER_TIMER_UNIT0_CORE1_STALL_EN_Pos 27
#define SYSTIMER_TIMER_UNIT0_CORE1_STALL_EN_Msk (1UL << SYSTIMER_TIMER_UNIT0_CORE1_STALL_EN_Pos)
#define SYSTIMER_TIMER_UNIT1_CORE0_STALL_EN_Pos 26
#define SYSTIMER_TIMER_UNIT1_CORE0_STALL_EN_Msk (1UL << SYSTIMER_TIMER_UNIT1_CORE0_STALL_EN_Pos)
#define SYSTIMER_TIMER_UNIT1_CORE1_STALL_EN_Pos 25
#define SYSTIMER_TIMER_UNIT1_CORE1_STALL_EN_Msk (1UL << SYSTIMER_TIMER_UNIT1_CORE1_STALL_EN_Pos)
#define SYSTIMER_TARGET0_WORK_EN_Pos            24
#define SYSTIMER_TARGET0_WORK_EN_Msk            (1UL << SYSTIMER_TARGET0_WORK_EN_Pos)
#define SYSTIMER_TARGET1_WORK_EN_Pos            23
#define SYSTIMER_TARGET1_WORK_EN_Msk            (1UL << SYSTIMER_TARGET1_WORK_EN_Pos)
#define SYSTIMER_TARGET2_WORK_EN_Pos            22
#define SYSTIMER_TARGET2_WORK_EN_Msk            (1UL << SYSTIMER_TARGET2_WORK_EN_Pos)
#define SYSTIMER_ETM_EN_Pos                     1
#define SYSTIMER_ETM_EN_Msk                     (1UL << SYSTIMER_ETM_EN_Pos)

/* Counter unit u (0/1). UNIT_OP triggers a snapshot into VALUE_HI/LO; LOAD_HI/LO
 * stage a reload value committed by the UNIT_LOAD strobe. */
#define SYSTIMER_UNIT_OP_REG(u)                 (0x0004 + 4 * (u))
#define SYSTIMER_UNIT_LOAD_HI_REG(u)            (0x000C + 8 * (u))
#define SYSTIMER_UNIT_LOAD_LO_REG(u)            (0x0010 + 8 * (u))
#define SYSTIMER_UNIT_VALUE_HI_REG(u)           (0x0040 + 8 * (u))
#define SYSTIMER_UNIT_VALUE_LO_REG(u)           (0x0044 + 8 * (u))
#define SYSTIMER_UNIT_LOAD_REG(u)               (0x005C + 4 * (u))

#define SYSTIMER_TIMER_UNIT_UPDATE_Pos          30
#define SYSTIMER_TIMER_UNIT_UPDATE_Msk          (1UL << SYSTIMER_TIMER_UNIT_UPDATE_Pos)
#define SYSTIMER_TIMER_UNIT_VALUE_VALID_Pos     29
#define SYSTIMER_TIMER_UNIT_VALUE_VALID_Msk     (1UL << SYSTIMER_TIMER_UNIT_VALUE_VALID_Pos)
#define SYSTIMER_TIMER_UNIT_LOAD_HI_Pos         0
#define SYSTIMER_TIMER_UNIT_LOAD_HI_Msk         (WHAL_BITMASK(20) << SYSTIMER_TIMER_UNIT_LOAD_HI_Pos)
#define SYSTIMER_TIMER_UNIT_VALUE_HI_Pos        0
#define SYSTIMER_TIMER_UNIT_VALUE_HI_Msk        (WHAL_BITMASK(20) << SYSTIMER_TIMER_UNIT_VALUE_HI_Pos)
#define SYSTIMER_TIMER_UNIT_LOAD_Pos            0
#define SYSTIMER_TIMER_UNIT_LOAD_Msk            (1UL << SYSTIMER_TIMER_UNIT_LOAD_Pos)

/* Comparator t (0/1/2). TARGET_HI/LO stage the alarm value committed by the
 * COMP_LOAD strobe; TARGET_CONF picks target/period mode and the source unit. */
#define SYSTIMER_TARGET_HI_REG(t)               (0x001C + 8 * (t))
#define SYSTIMER_TARGET_LO_REG(t)               (0x0020 + 8 * (t))
#define SYSTIMER_TARGET_CONF_REG(t)             (0x0034 + 4 * (t))
#define SYSTIMER_COMP_LOAD_REG(t)               (0x0050 + 4 * (t))

#define SYSTIMER_TIMER_TARGET_HI_Pos            0
#define SYSTIMER_TIMER_TARGET_HI_Msk            (WHAL_BITMASK(20) << SYSTIMER_TIMER_TARGET_HI_Pos)
#define SYSTIMER_TARGET_PERIOD_Pos              0
#define SYSTIMER_TARGET_PERIOD_Msk              (WHAL_BITMASK(26) << SYSTIMER_TARGET_PERIOD_Pos)
#define SYSTIMER_TARGET_PERIOD_MODE_Pos         30
#define SYSTIMER_TARGET_PERIOD_MODE_Msk         (1UL << SYSTIMER_TARGET_PERIOD_MODE_Pos)
#define SYSTIMER_TARGET_TIMER_UNIT_SEL_Pos      31
#define SYSTIMER_TARGET_TIMER_UNIT_SEL_Msk      (1UL << SYSTIMER_TARGET_TIMER_UNIT_SEL_Pos)
#define SYSTIMER_TIMER_COMP_LOAD_Pos            0
#define SYSTIMER_TIMER_COMP_LOAD_Msk            (1UL << SYSTIMER_TIMER_COMP_LOAD_Pos)

/* Interrupt registers: one bit per comparator, same layout in ENA/RAW/CLR/ST. */
#define SYSTIMER_INT_ENA_REG                    0x0064
#define SYSTIMER_INT_RAW_REG                    0x0068
#define SYSTIMER_INT_CLR_REG                    0x006C
#define SYSTIMER_INT_ST_REG                     0x0070
#define SYSTIMER_TARGET0_INT_Pos                0
#define SYSTIMER_TARGET0_INT_Msk                (1UL << SYSTIMER_TARGET0_INT_Pos)
#define SYSTIMER_TARGET1_INT_Pos                1
#define SYSTIMER_TARGET1_INT_Msk                (1UL << SYSTIMER_TARGET1_INT_Pos)
#define SYSTIMER_TARGET2_INT_Pos                2
#define SYSTIMER_TARGET2_INT_Msk                (1UL << SYSTIMER_TARGET2_INT_Pos)

#ifdef WHAL_CFG_ESP32C6_SYSTIMER_DIRECT_API_MAPPING
#define whal_Esp32c6_Systimer_Init   whal_Timer_Init
#define whal_Esp32c6_Systimer_Deinit whal_Timer_Deinit
#define whal_Esp32c6_Systimer_Start  whal_Timer_Start
#define whal_Esp32c6_Systimer_Stop   whal_Timer_Stop
#define whal_Esp32c6_Systimer_Reset  whal_Timer_Reset
#endif /* WHAL_CFG_ESP32C6_SYSTIMER_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_ESP32C6_SYSTIMER_SINGLE_INSTANCE
const whal_Timer whal_Esp32c6_Systimer_Dev = WHAL_CFG_ESP32C6_SYSTIMER_DEV;
#endif

/* Per-instance CONF work-enable and INT-enable bits, indexed by unit/comp.
 * The named field macros are per-instance (distinct bits in a shared register),
 * so map the unit/comp index to the right mask through these tables. */
static const uint32_t unitWorkEnMsk[] = {
    SYSTIMER_TIMER_UNIT0_WORK_EN_Msk,
    SYSTIMER_TIMER_UNIT1_WORK_EN_Msk,
};
static const uint32_t compWorkEnMsk[] = {
    SYSTIMER_TARGET0_WORK_EN_Msk,
    SYSTIMER_TARGET1_WORK_EN_Msk,
    SYSTIMER_TARGET2_WORK_EN_Msk,
};
static const uint32_t compIntMsk[] = {
    SYSTIMER_TARGET0_INT_Msk,
    SYSTIMER_TARGET1_INT_Msk,
    SYSTIMER_TARGET2_INT_Msk,
};

whal_Error whal_Esp32c6_Systimer_Init(whal_Timer *timerDev)
{
#ifdef WHAL_CFG_ESP32C6_SYSTIMER_SINGLE_INSTANCE
    const whal_Esp32c6_Systimer_Cfg *cfg =
        (const whal_Esp32c6_Systimer_Cfg *)whal_Esp32c6_Systimer_Dev.cfg;
    size_t base = whal_Esp32c6_Systimer_Dev.base;
    (void)timerDev;
#else
    const whal_Esp32c6_Systimer_Cfg *cfg;
    size_t base;

    if (!timerDev || !timerDev->cfg) {
        return WHAL_EINVAL;
    }

    base = timerDev->base;
    cfg = (const whal_Esp32c6_Systimer_Cfg *)timerDev->cfg;
#endif

    if (cfg->unit > WHAL_ESP32C6_SYSTIMER_UNIT1 ||
        cfg->comp > WHAL_ESP32C6_SYSTIMER_COMP2) {
        return WHAL_EINVAL;
    }

    if (cfg->mode == WHAL_ESP32C6_SYSTIMER_PERIODIC) {
        /* Period mode (TRM 13.5.3): select unit + period, load, then set mode. */
        whal_Reg_Write(base, SYSTIMER_TARGET_CONF_REG(cfg->comp),
            whal_SetBits(SYSTIMER_TARGET_TIMER_UNIT_SEL_Msk,
                         SYSTIMER_TARGET_TIMER_UNIT_SEL_Pos, cfg->unit) |
            whal_SetBits(SYSTIMER_TARGET_PERIOD_Msk,
                         SYSTIMER_TARGET_PERIOD_Pos, cfg->period));
        whal_Reg_Write(base, SYSTIMER_COMP_LOAD_REG(cfg->comp),
                       SYSTIMER_TIMER_COMP_LOAD_Msk);
        whal_Reg_Update(base, SYSTIMER_TARGET_CONF_REG(cfg->comp),
                        SYSTIMER_TARGET_PERIOD_MODE_Msk,
                        SYSTIMER_TARGET_PERIOD_MODE_Msk);
    } else {
        /* Target mode (TRM 13.5.2): unit select (period mode cleared), then the
         * absolute 52-bit alarm value across HI/LO, then load. */
        whal_Reg_Write(base, SYSTIMER_TARGET_CONF_REG(cfg->comp),
            whal_SetBits(SYSTIMER_TARGET_TIMER_UNIT_SEL_Msk,
                         SYSTIMER_TARGET_TIMER_UNIT_SEL_Pos, cfg->unit));
        whal_Reg_Write(base, SYSTIMER_TARGET_HI_REG(cfg->comp),
            (uint32_t)(cfg->compValue >> 32) & SYSTIMER_TIMER_TARGET_HI_Msk);
        whal_Reg_Write(base, SYSTIMER_TARGET_LO_REG(cfg->comp),
            (uint32_t)cfg->compValue);
        whal_Reg_Write(base, SYSTIMER_COMP_LOAD_REG(cfg->comp),
                       SYSTIMER_TIMER_COMP_LOAD_Msk);
    }

    /* Enable this comparator's alarm interrupt. */
    whal_Reg_Update(base, SYSTIMER_INT_ENA_REG,
                    compIntMsk[cfg->comp], compIntMsk[cfg->comp]);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Systimer_Deinit(whal_Timer *timerDev)
{
#ifdef WHAL_CFG_ESP32C6_SYSTIMER_SINGLE_INSTANCE
    const whal_Esp32c6_Systimer_Cfg *cfg =
        (const whal_Esp32c6_Systimer_Cfg *)whal_Esp32c6_Systimer_Dev.cfg;
    size_t base = whal_Esp32c6_Systimer_Dev.base;
    (void)timerDev;
#else
    const whal_Esp32c6_Systimer_Cfg *cfg;
    size_t base;

    if (!timerDev || !timerDev->cfg) {
        return WHAL_EINVAL;
    }

    base = timerDev->base;
    cfg = (const whal_Esp32c6_Systimer_Cfg *)timerDev->cfg;
#endif

    if (cfg->comp > WHAL_ESP32C6_SYSTIMER_COMP2) {
        return WHAL_EINVAL;
    }

    /* Disable this comparator and its interrupt (leave the shared counter). */
    whal_Reg_Update(base, SYSTIMER_CONF_REG, compWorkEnMsk[cfg->comp], 0);
    whal_Reg_Update(base, SYSTIMER_INT_ENA_REG, compIntMsk[cfg->comp], 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Systimer_Start(whal_Timer *timerDev)
{
#ifdef WHAL_CFG_ESP32C6_SYSTIMER_SINGLE_INSTANCE
    const whal_Esp32c6_Systimer_Cfg *cfg =
        (const whal_Esp32c6_Systimer_Cfg *)whal_Esp32c6_Systimer_Dev.cfg;
    size_t base = whal_Esp32c6_Systimer_Dev.base;
    uint32_t enMsk;
    (void)timerDev;
#else
    const whal_Esp32c6_Systimer_Cfg *cfg;
    size_t base;
    uint32_t enMsk;

    if (!timerDev || !timerDev->cfg) {
        return WHAL_EINVAL;
    }

    base = timerDev->base;
    cfg = (const whal_Esp32c6_Systimer_Cfg *)timerDev->cfg;
#endif

    if (cfg->unit > WHAL_ESP32C6_SYSTIMER_UNIT1 ||
        cfg->comp > WHAL_ESP32C6_SYSTIMER_COMP2) {
        return WHAL_EINVAL;
    }

    /* Run the shared counter and enable this comparator. */
    enMsk = unitWorkEnMsk[cfg->unit] | compWorkEnMsk[cfg->comp];
    whal_Reg_Update(base, SYSTIMER_CONF_REG, enMsk, enMsk);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Systimer_Stop(whal_Timer *timerDev)
{
#ifdef WHAL_CFG_ESP32C6_SYSTIMER_SINGLE_INSTANCE
    const whal_Esp32c6_Systimer_Cfg *cfg =
        (const whal_Esp32c6_Systimer_Cfg *)whal_Esp32c6_Systimer_Dev.cfg;
    size_t base = whal_Esp32c6_Systimer_Dev.base;
    (void)timerDev;
#else
    const whal_Esp32c6_Systimer_Cfg *cfg;
    size_t base;

    if (!timerDev || !timerDev->cfg) {
        return WHAL_EINVAL;
    }

    base = timerDev->base;
    cfg = (const whal_Esp32c6_Systimer_Cfg *)timerDev->cfg;
#endif

    if (cfg->comp > WHAL_ESP32C6_SYSTIMER_COMP2) {
        return WHAL_EINVAL;
    }

    /* Disable only this comparator; the shared counter keeps running for any
     * other comparators riding it. */
    whal_Reg_Update(base, SYSTIMER_CONF_REG, compWorkEnMsk[cfg->comp], 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Systimer_Reset(whal_Timer *timerDev)
{
#ifdef WHAL_CFG_ESP32C6_SYSTIMER_SINGLE_INSTANCE
    const whal_Esp32c6_Systimer_Cfg *cfg =
        (const whal_Esp32c6_Systimer_Cfg *)whal_Esp32c6_Systimer_Dev.cfg;
    size_t base = whal_Esp32c6_Systimer_Dev.base;
    (void)timerDev;
#else
    const whal_Esp32c6_Systimer_Cfg *cfg;
    size_t base;

    if (!timerDev || !timerDev->cfg) {
        return WHAL_EINVAL;
    }

    base = timerDev->base;
    cfg = (const whal_Esp32c6_Systimer_Cfg *)timerDev->cfg;
#endif

    if (cfg->unit > WHAL_ESP32C6_SYSTIMER_UNIT1) {
        return WHAL_EINVAL;
    }

    /* Reload the shared counter to 0. This affects every comparator on this
     * unit, since they share the one time base. */
    whal_Reg_Write(base, SYSTIMER_UNIT_LOAD_HI_REG(cfg->unit), 0);
    whal_Reg_Write(base, SYSTIMER_UNIT_LOAD_LO_REG(cfg->unit), 0);
    whal_Reg_Write(base, SYSTIMER_UNIT_LOAD_REG(cfg->unit),
                   SYSTIMER_TIMER_UNIT_LOAD_Msk);

    return WHAL_SUCCESS;
}

void whal_Esp32c6_Systimer_IrqHandler(whal_Timer *timerDev)
{
#ifdef WHAL_CFG_ESP32C6_SYSTIMER_SINGLE_INSTANCE
    const whal_Esp32c6_Systimer_Cfg *cfg =
        (const whal_Esp32c6_Systimer_Cfg *)whal_Esp32c6_Systimer_Dev.cfg;
    size_t base = whal_Esp32c6_Systimer_Dev.base;
    (void)timerDev;
#else
    const whal_Esp32c6_Systimer_Cfg *cfg;
    size_t base;

    if (!timerDev || !timerDev->cfg) {
        return;
    }

    base = timerDev->base;
    cfg = (const whal_Esp32c6_Systimer_Cfg *)timerDev->cfg;
#endif

    if (cfg->comp > WHAL_ESP32C6_SYSTIMER_COMP2) {
        return;
    }

    whal_Reg_Write(base, SYSTIMER_INT_CLR_REG, compIntMsk[cfg->comp]);
}

#ifndef WHAL_CFG_ESP32C6_SYSTIMER_DIRECT_API_MAPPING
const whal_TimerDriver whal_Esp32c6_Systimer_Driver = {
    .Init = whal_Esp32c6_Systimer_Init,
    .Deinit = whal_Esp32c6_Systimer_Deinit,
    .Start = whal_Esp32c6_Systimer_Start,
    .Stop = whal_Esp32c6_Systimer_Stop,
    .Reset = whal_Esp32c6_Systimer_Reset,
};
#endif /* !WHAL_CFG_ESP32C6_SYSTIMER_DIRECT_API_MAPPING */
