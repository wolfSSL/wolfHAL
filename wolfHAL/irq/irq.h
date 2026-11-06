/* irq.h
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

#ifndef WHAL_IRQ_H
#define WHAL_IRQ_H

#include <wolfHAL/error.h>
#include <stddef.h>
#include <stdint.h>

/*
 * @file irq.h
 * @brief Generic interrupt controller abstraction and driver interface.
 *
 * A whal_Irq device represents an interrupt controller (e.g., ARM Cortex-M
 * NVIC). The API allows enabling and disabling individual interrupt lines,
 * with optional platform-specific configuration (e.g., priority) passed
 * through the Enable call.
 */

typedef struct whal_Irq whal_Irq;

/*
 * @brief Driver vtable for interrupt controllers.
 */
typedef struct {
    /* Initialize the interrupt controller. */
    whal_Error (*Init)(whal_Irq *irqDev);
    /* Deinitialize the interrupt controller. */
    whal_Error (*Deinit)(whal_Irq *irqDev);
    /* Enable an interrupt line. irqCfg is platform-specific (or NULL). */
    whal_Error (*Enable)(whal_Irq *irqDev, size_t irq, const void *irqCfg);
    /* Disable an interrupt line. */
    whal_Error (*Disable)(whal_Irq *irqDev, size_t irq);
} whal_IrqDriver;

/*
 * @brief Interrupt controller device instance.
 */
struct whal_Irq {
    const uintptr_t base;
    const whal_IrqDriver *driver;
    const void *cfg;
};

/*
 * @brief Initialize the interrupt controller.
 *
 * @param irqDev Interrupt controller instance.
 *
 * @retval WHAL_SUCCESS Init completed.
 * @retval WHAL_EINVAL  Null pointer or missing driver function.
 */
whal_Error whal_Irq_Init(whal_Irq *irqDev);
/*
 * @brief Deinitialize the interrupt controller.
 *
 * @param irqDev Interrupt controller instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Null pointer or missing driver function.
 */
whal_Error whal_Irq_Deinit(whal_Irq *irqDev);
/*
 * @brief Enable an interrupt line.
 *
 * @param irqDev Interrupt controller instance.
 * @param irq    Interrupt number.
 * @param irqCfg Platform-specific config (e.g., priority), or NULL for defaults.
 *
 * @retval WHAL_SUCCESS Interrupt enabled.
 * @retval WHAL_EINVAL  Null pointer or missing driver function.
 */
whal_Error whal_Irq_Enable(whal_Irq *irqDev, size_t irq, const void *irqCfg);
/*
 * @brief Disable an interrupt line.
 *
 * @param irqDev Interrupt controller instance.
 * @param irq    Interrupt number.
 *
 * @retval WHAL_SUCCESS Interrupt disabled.
 * @retval WHAL_EINVAL  Null pointer or missing driver function.
 */
whal_Error whal_Irq_Disable(whal_Irq *irqDev, size_t irq);

#endif /* WHAL_IRQ_H */
