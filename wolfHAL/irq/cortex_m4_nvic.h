/* cortex_m4_nvic.h
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

#ifndef WHAL_CORTEX_M4_NVIC_H
#define WHAL_CORTEX_M4_NVIC_H

#include <wolfHAL/irq/irq.h>
#include <stdint.h>

/*
 * @file cortex_m4_nvic.h
 * @brief ARM Cortex-M4 NVIC driver.
 *
 * Uses a 4-bit priority field in bits[7:4] of each IPR byte.
 * The regmap base should be 0xE000E100 (NVIC_ISER0).
 */

/*
 * @brief Optional per-interrupt configuration.
 *
 * Pass to whal_Irq_Enable() to set priority, or pass NULL for default (0).
 */
typedef struct {
    uint8_t priority;
} whal_Nvic_Cfg;

/*
 * @brief Platform-owned NVIC device singleton. Defined in the driver TU
 * from the WHAL_CFG_NVIC_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Irq whal_Nvic_Dev;

#ifndef WHAL_CFG_NVIC_IRQ_DIRECT_API_MAPPING
/*
 * @brief Driver instance for Cortex-M4 NVIC.
 */
extern const whal_IrqDriver whal_Nvic_Driver;

/*
 * @brief Initialize the Cortex-M4 NVIC driver.
 *
 * @param irqDev IRQ device instance.
 *
 * @retval WHAL_SUCCESS NVIC is ready.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Nvic_Init(whal_Irq *irqDev);

/*
 * @brief Deinitialize the NVIC driver.
 *
 * @param irqDev IRQ device instance.
 *
 * @retval WHAL_SUCCESS Driver deinitialized.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Nvic_Deinit(whal_Irq *irqDev);

/*
 * @brief Enable an IRQ in the NVIC and optionally set its priority.
 *
 * @param irqDev IRQ device instance.
 * @param irq    Interrupt number.
 * @param irqCfg Optional pointer to a `whal_Nvic_Cfg` for priority. Pass
 *               NULL to leave the priority at its current value.
 *
 * @retval WHAL_SUCCESS Interrupt enabled.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Nvic_Enable(whal_Irq *irqDev, size_t irq, const void *irqCfg);

/*
 * @brief Disable an IRQ in the NVIC.
 *
 * @param irqDev IRQ device instance.
 * @param irq    Interrupt number.
 *
 * @retval WHAL_SUCCESS Interrupt disabled.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Nvic_Disable(whal_Irq *irqDev, size_t irq);
#endif /* !WHAL_CFG_NVIC_IRQ_DIRECT_API_MAPPING */

#endif /* WHAL_CORTEX_M4_NVIC_H */
