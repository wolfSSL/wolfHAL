/* versal2.h
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

#ifndef WHAL_VERSAL2_H
#define WHAL_VERSAL2_H

#include <wolfHAL/uart/pl011_uart.h>
#include <wolfHAL/spimem/cadence_ospi_spimem.h>

/*
 * @file versal2.h
 * @brief Convenience initializers for AMD Versal Gen 2 (AI Edge Series
 *        Gen 2 / Prime Series Gen 2) device instances.
 */

/* LPD UARTs, Arm PL011 (AM026 ch. 63; bases from AM028 UART module). */
#define WHAL_VERSAL2_UART0_BASE 0xF1920000
#define WHAL_VERSAL2_UART1_BASE 0xF1930000
#define WHAL_VERSAL2_UART_DRIVER &whal_Pl011_Uart_Driver

/* OSPI flash controller (Cadence OSPI, AM026 ch. 66, AM028 OSPI module). */
#define WHAL_VERSAL2_OSPI_BASE   0xF1010000
#define WHAL_VERSAL2_OSPI_DRIVER &whal_CadenceOspi_SpiMem_Driver

/* OSPI DMA indirect read values (AM026 Table 223, p707-708). Trigger base
 * is the flash window (p704); its 64 B must not cross 4 KB (p709). */
#define WHAL_VERSAL2_OSPI_IND_TRIG_BASE   0xC0000000 /* indTrigBase */
#define WHAL_VERSAL2_OSPI_IND_TRIG_RANGE  0x6        /* indTrigRange: 64 B */
#define WHAL_VERSAL2_OSPI_SRAM_PARTITION  0xFE       /* sramPartition */
#define WHAL_VERSAL2_OSPI_DMA_PERIPH_CFG  0x602      /* dmaPeriphCfg */

/* OSPI direct mode window: 0xC000_0000 to 0xDFFF_FFFF (AM026 p704). Its
 * AXI port is shared with the OSPI DMA, switched by
 * whal_Versal2_Ospi_MapSelect (versal2_ospi_dma.h). */
#define WHAL_VERSAL2_OSPI_MAP_BASE 0xC0000000
#define WHAL_VERSAL2_OSPI_MAP_SIZE 0x20000000

/* DEV_SIZE_CONFIG for the VEK385 MT35XU02G: 2 Gb on CS0, subsector 16,
 * 256 B pages, 4 address bytes (AM028 dev_size_config_reg). */
#define WHAL_VERSAL2_OSPI_DEV_SIZE_VEK385 \
    ((2UL << 21) | (0x10UL << 16) | (0x100UL << 4) | 3UL)

#endif /* WHAL_VERSAL2_H */
