/* main.c
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

#include <stdint.h>
#include <wolfHAL/wolfHAL.h>
#include "wolfHAL_board.h"

void main(void)
{
    if (Board_Init() != WHAL_SUCCESS)
        goto loop;

    while (1) {
        whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1);
        whal_Uart_Send(BOARD_UART_DEV, "Blink!\r\n", 8);
        Board_WaitMs(1000);
        whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 0);
        whal_Uart_Send(BOARD_UART_DEV, "Blink!\r\n", 8);
        Board_WaitMs(1000);
    }

loop:
    while (1);
}
