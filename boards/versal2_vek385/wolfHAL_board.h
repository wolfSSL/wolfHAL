/* wolfHAL_board.h
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

#ifndef WOLFHAL_BOARD_H
#define WOLFHAL_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/platform/amd/versal2.h>

/* UART reference clock and console baud rate. The clock is the VEK385
 * device tree's xlnx,clock-freq for the LPD UARTs. */
#define BOARD_UART_CLK_HZ 99999001
#define BOARD_UART_BAUD   115200

extern whal_Uart g_whalUart;
extern whal_Timeout g_whalTimeout;

/* No LED driver: the test runner's whal_Gpio_Set calls return
 * WHAL_EINVAL and do nothing. */
#define BOARD_LED_PIN 0

/* BOARD_*_DEV: how this board reaches each peripheral. UART1 is the
 * console (the VEK385 FTDI port, QEMU's second serial). */
#define BOARD_GPIO_DEV WHAL_INTERNAL_DEV
#define BOARD_UART_DEV (&g_whalUart)

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* WOLFHAL_BOARD_H */
