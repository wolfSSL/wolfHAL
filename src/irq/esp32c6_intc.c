/* esp32c6_intc.c
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

#include "wolfHAL_board.h"  /* provides WHAL_CFG_ESP32C6_INTC_DEV initializer */
#include <wolfHAL/irq/esp32c6_intc.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

const whal_Irq whal_Esp32c6_Intc_Dev = WHAL_CFG_ESP32C6_INTC_DEV;

/* Interrupt matrix (TRM chapter 10), relative to the device base. One map
 * register per peripheral source, holding the CPU line it routes to (0 =
 * unrouted). */
#define INTMTX_SOURCE_MAP_REG(src)      (0x0000 + 4 * (src))
#define INTMTX_SOURCE_MAP_Pos           0
#define INTMTX_SOURCE_MAP_Msk           (WHAL_BITMASK(5) << INTMTX_SOURCE_MAP_Pos)

/* CPU interrupt controller (PLIC MX), relative to plicMxBase. The TRM
 * documents the equivalent INTPRI registers at 0x600C_5000, but those do not
 * drive the HP CPU; ENABLE and TYPE hold one bit per CPU line. */
#define PLIC_MXINT_ENABLE_REG           0x0000
#define PLIC_MXINT_TYPE_REG             0x0004
#define PLIC_MXINT_PRI_REG(n)           (0x0010 + 4 * (n))
#define PLIC_MXINT_PRI_Pos              0
#define PLIC_MXINT_PRI_Msk              (WHAL_BITMASK(4) << PLIC_MXINT_PRI_Pos)
#define PLIC_MXINT_THRESH_REG           0x0090
#define PLIC_MXINT_THRESH_Pos           0
#define PLIC_MXINT_THRESH_Msk           (WHAL_BITMASK(8) << PLIC_MXINT_THRESH_Pos)

/* Lines 0, 3, 4 and 7 are core-local (CLINT) and not routable. */
#define INTC_CLINT_LINES_Msk            ((1UL << 0) | (1UL << 3) | (1UL << 4) | (1UL << 7))
#define INTC_PRIORITY_MAX               15

#define MSTATUS_MIE_Msk                 (1UL << 3)

#ifdef WHAL_CFG_ESP32C6_INTC_DIRECT_API_MAPPING
#define whal_Esp32c6_Intc_Init    whal_Irq_Init
#define whal_Esp32c6_Intc_Deinit  whal_Irq_Deinit
#define whal_Esp32c6_Intc_Enable  whal_Irq_Enable
#define whal_Esp32c6_Intc_Disable whal_Irq_Disable
#endif /* WHAL_CFG_ESP32C6_INTC_DIRECT_API_MAPPING */

whal_Error whal_Esp32c6_Intc_Init(whal_Irq *irqDev)
{
    const whal_Esp32c6_Intc_DevCfg *devCfg =
        (const whal_Esp32c6_Intc_DevCfg *)whal_Esp32c6_Intc_Dev.cfg;
    (void)irqDev;

    whal_Reg_Update(devCfg->plicMxBase, PLIC_MXINT_THRESH_REG,
                    PLIC_MXINT_THRESH_Msk,
                    whal_SetBits(PLIC_MXINT_THRESH_Msk,
                                 PLIC_MXINT_THRESH_Pos, 1));
    __asm__ volatile("fence" ::: "memory");
    __asm__ volatile("csrs mstatus, %0" :: "r"(MSTATUS_MIE_Msk) : "memory");

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Intc_Deinit(whal_Irq *irqDev)
{
    (void)irqDev;

    __asm__ volatile("csrc mstatus, %0" :: "r"(MSTATUS_MIE_Msk) : "memory");

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Intc_Enable(whal_Irq *irqDev, size_t irq,
                                    const void *irqCfg)
{
    const whal_Esp32c6_Intc_DevCfg *devCfg =
        (const whal_Esp32c6_Intc_DevCfg *)whal_Esp32c6_Intc_Dev.cfg;
    size_t base = whal_Esp32c6_Intc_Dev.base;
    const whal_Esp32c6_Intc_Cfg *cfg = (const whal_Esp32c6_Intc_Cfg *)irqCfg;
    size_t lineMsk;
    size_t mstatus;
    (void)irqDev;

    if (!cfg) {
        return WHAL_EINVAL;
    }

    if (irq >= WHAL_ESP32C6_INTSRC_COUNT || cfg->cpuLine > 31 ||
        (INTC_CLINT_LINES_Msk & (1UL << cfg->cpuLine)) ||
        cfg->priority < 1 || cfg->priority > INTC_PRIORITY_MAX) {
        return WHAL_ENOTSUP;
    }

    lineMsk = 1UL << cfg->cpuLine;

    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(mstatus) : "r"(MSTATUS_MIE_Msk) : "memory");

    whal_Reg_Update(devCfg->plicMxBase, PLIC_MXINT_TYPE_REG, lineMsk, 0);
    whal_Reg_Update(devCfg->plicMxBase, PLIC_MXINT_PRI_REG(cfg->cpuLine),
                    PLIC_MXINT_PRI_Msk,
                    whal_SetBits(PLIC_MXINT_PRI_Msk, PLIC_MXINT_PRI_Pos,
                                 cfg->priority));
    whal_Reg_Update(devCfg->plicMxBase, PLIC_MXINT_ENABLE_REG,
                    lineMsk, lineMsk);
    whal_Reg_Update(base, INTMTX_SOURCE_MAP_REG(irq), INTMTX_SOURCE_MAP_Msk,
                    whal_SetBits(INTMTX_SOURCE_MAP_Msk, INTMTX_SOURCE_MAP_Pos,
                                 cfg->cpuLine));
    __asm__ volatile("csrs mie, %0" :: "r"(lineMsk) : "memory");
    __asm__ volatile("fence" ::: "memory");

    __asm__ volatile("csrs mstatus, %0"
                     :: "r"(mstatus & MSTATUS_MIE_Msk) : "memory");

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Intc_Disable(whal_Irq *irqDev, size_t irq)
{
    size_t base = whal_Esp32c6_Intc_Dev.base;
    size_t mstatus;
    (void)irqDev;

    if (irq >= WHAL_ESP32C6_INTSRC_COUNT) {
        return WHAL_ENOTSUP;
    }

    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(mstatus) : "r"(MSTATUS_MIE_Msk) : "memory");

    whal_Reg_Update(base, INTMTX_SOURCE_MAP_REG(irq), INTMTX_SOURCE_MAP_Msk, 0);
    __asm__ volatile("fence" ::: "memory");

    __asm__ volatile("csrs mstatus, %0"
                     :: "r"(mstatus & MSTATUS_MIE_Msk) : "memory");

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_ESP32C6_INTC_DIRECT_API_MAPPING
const whal_IrqDriver whal_Esp32c6_Intc_Driver = {
    .Init = whal_Esp32c6_Intc_Init,
    .Deinit = whal_Esp32c6_Intc_Deinit,
    .Enable = whal_Esp32c6_Intc_Enable,
    .Disable = whal_Esp32c6_Intc_Disable,
};
#endif /* !WHAL_CFG_ESP32C6_INTC_DIRECT_API_MAPPING */
