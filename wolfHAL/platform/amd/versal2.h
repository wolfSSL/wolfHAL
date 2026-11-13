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

/*
 * @file versal2.h
 * @brief Convenience initializers for AMD Versal Gen 2 (AI Edge Series
 *        Gen 2 / Prime Series Gen 2) device instances.
 */

/* LPD UARTs, Arm PL011 (AM026 ch. 63; bases from AM028 UART module). */
#define WHAL_VERSAL2_UART0_BASE 0xF1920000
#define WHAL_VERSAL2_UART1_BASE 0xF1930000
#define WHAL_VERSAL2_UART_DRIVER &whal_Pl011_Uart_Driver

#endif /* WHAL_VERSAL2_H */
