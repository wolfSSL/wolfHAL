/* esp32c6_intc.h
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

#ifndef WHAL_ESP32C6_INTC_H
#define WHAL_ESP32C6_INTC_H

#include <stddef.h>
#include <stdint.h>
#include <wolfHAL/irq/irq.h>

/*
 * @file esp32c6_intc.h
 * @brief ESP32-C6 interrupt controller driver.
 *
 * Covers the interrupt matrix (INTMTX), which routes each of the 77
 * peripheral interrupt sources to one of the CPU interrupt lines, and the
 * CPU interrupt controller (PLIC MX), which holds each line's enable, type and
 * priority. The irq argument to Enable/Disable is the peripheral source
 * number (TRM Table 10.3-1). Lines 0, 3, 4 and 7 are reserved for core-local
 * interrupts and cannot be used.
 */

/* Number of peripheral interrupt sources in the interrupt matrix. */
#define WHAL_ESP32C6_INTSRC_COUNT 77

/*
 * @brief Device configuration.
 */
typedef struct {
    size_t plicMxBase; /* PLIC MX register base; the device base is INTMTX */
} whal_Esp32c6_Intc_DevCfg;

/*
 * @brief Per-interrupt configuration passed to whal_Irq_Enable().
 */
typedef struct {
    uint8_t cpuLine;  /* CPU interrupt line: 1, 2, 5, 6 or 8-31 */
    uint8_t priority; /* 1 (lowest) to 15 (highest) */
} whal_Esp32c6_Intc_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_ESP32C6_INTC_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Irq whal_Esp32c6_Intc_Dev;

#ifndef WHAL_CFG_ESP32C6_INTC_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the ESP32-C6 interrupt controller.
 */
extern const whal_IrqDriver whal_Esp32c6_Intc_Driver;

/*
 * @brief Mask priority-0 lines and enable interrupts globally (mstatus.MIE).
 *
 * @param irqDev IRQ device instance.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_Intc_Init(whal_Irq *irqDev);

/*
 * @brief Disable interrupts globally (mstatus.MIE).
 *
 * @param irqDev IRQ device instance.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_Intc_Deinit(whal_Irq *irqDev);

/*
 * @brief Route a peripheral interrupt source to a CPU line and enable that
 * line as level-triggered at the given priority.
 *
 * @param irqDev IRQ device instance.
 * @param irq    Peripheral interrupt source number.
 * @param irqCfg Pointer to a whal_Esp32c6_Intc_Cfg.
 *
 * @retval WHAL_SUCCESS Source routed and line enabled.
 * @retval WHAL_EINVAL  irqCfg is NULL.
 * @retval WHAL_ENOTSUP Source, CPU line or priority out of range.
 */
whal_Error whal_Esp32c6_Intc_Enable(whal_Irq *irqDev, size_t irq, const void *irqCfg);

/*
 * @brief Unroute a peripheral interrupt source. The CPU line stays enabled
 * since other sources may share it.
 *
 * @param irqDev IRQ device instance.
 * @param irq    Peripheral interrupt source number.
 *
 * @retval WHAL_SUCCESS Source unrouted.
 * @retval WHAL_ENOTSUP Source out of range.
 */
whal_Error whal_Esp32c6_Intc_Disable(whal_Irq *irqDev, size_t irq);
#endif /* !WHAL_CFG_ESP32C6_INTC_DIRECT_API_MAPPING */

#endif /* WHAL_ESP32C6_INTC_H */
