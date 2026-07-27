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

/*
 * Barebones ESP32-C6 (RV32IMAC) application.
 *
 * Reached from Reset_Handler() in ivt.c once the C runtime is up. Nothing is
 * wired yet, so just spin. The counter is `volatile` so the compiler keeps the
 * loop body, giving a live variable to watch on a debugger or over JTAG.
 */
void main(void)
{
    volatile uint32_t ticks = 0;

    while (1) {
        ticks++;
    }
}
