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

#include "board.h"

/*
 * Barebones ESP32-C6 (RV32IMAC) application.
 *
 * Reached from Reset_Handler() in ivt.c once the C runtime is up. Bring up the
 * board (clocks + GPIO), drive the GPIO8 output high, then spin.
 */
void main(void)
{
    Board_Init();

    whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1);

    while (1) {
    }
}
