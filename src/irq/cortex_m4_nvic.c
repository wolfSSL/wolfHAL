/* cortex_m4_nvic.c
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

#include "wolfHAL_board.h"  /* provides WHAL_CFG_NVIC_DEV initializer */
#include <wolfHAL/irq/cortex_m4_nvic.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>

const whal_Irq whal_Nvic_Dev = WHAL_CFG_NVIC_DEV;

/*
 * ARM Cortex-M4 NVIC register offsets (relative to 0xE000E100).
 *
 * ISER: Interrupt Set-Enable Registers (0x000-0x01C, 32 IRQs per register)
 * ICER: Interrupt Clear-Enable Registers (0x080-0x09C)
 * IPR:  Interrupt Priority Registers (0x300-0x37F, 4 IRQs per register)
 */
#define NVIC_ISER_REG(irq) (0x000 + (((irq) >> 5) << 2))
#define NVIC_ICER_REG(irq) (0x080 + (((irq) >> 5) << 2))
#define NVIC_IPR_REG(irq)    (0x300 + (((irq) >> 2) << 2))
#define NVIC_IPR_SHIFT(irq)  (((irq) & 0x3) << 3)

#ifdef WHAL_CFG_NVIC_IRQ_DIRECT_API_MAPPING
#define whal_Nvic_Init    whal_Irq_Init
#define whal_Nvic_Deinit  whal_Irq_Deinit
#define whal_Nvic_Enable  whal_Irq_Enable
#define whal_Nvic_Disable whal_Irq_Disable
#endif /* WHAL_CFG_NVIC_IRQ_DIRECT_API_MAPPING */

whal_Error whal_Nvic_Init(whal_Irq *irqDev)
{
    (void)irqDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Nvic_Deinit(whal_Irq *irqDev)
{
    (void)irqDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Nvic_Enable(whal_Irq *irqDev, size_t irq,
                                    const void *irqCfg)
{
    size_t base = whal_Nvic_Dev.base;
    (void)irqDev;

    /* Set priority if config provided */
    if (irqCfg) {
        const whal_Nvic_Cfg *cfg = (const whal_Nvic_Cfg *)irqCfg;
        size_t shift = NVIC_IPR_SHIFT(irq);
        size_t mask = (0xFFUL << shift);
        whal_Reg_Update(base, NVIC_IPR_REG(irq), mask,
                        (size_t)(cfg->priority << 4) << shift);
    }

    /* Enable the interrupt */
    whal_Reg_Write(base, NVIC_ISER_REG(irq), (1UL << (irq & 0x1F)));

    return WHAL_SUCCESS;
}

whal_Error whal_Nvic_Disable(whal_Irq *irqDev, size_t irq)
{
    size_t base = whal_Nvic_Dev.base;
    (void)irqDev;

    whal_Reg_Write(base, NVIC_ICER_REG(irq), (1UL << (irq & 0x1F)));

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_NVIC_IRQ_DIRECT_API_MAPPING
const whal_IrqDriver whal_Nvic_Driver = {
    .Init = whal_Nvic_Init,
    .Deinit = whal_Nvic_Deinit,
    .Enable = whal_Nvic_Enable,
    .Disable = whal_Nvic_Disable,
};
#endif /* !WHAL_CFG_NVIC_IRQ_DIRECT_API_MAPPING */
