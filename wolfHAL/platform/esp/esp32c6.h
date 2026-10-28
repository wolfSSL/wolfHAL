/* esp32c6.h
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

#ifndef WHAL_ESP32C6_H
#define WHAL_ESP32C6_H

#include <wolfHAL/clock/esp32c6_clock.h>
#include <wolfHAL/gpio/esp32c6_gpio.h>
#include <wolfHAL/uart/esp32c6_uart.h>
#include <wolfHAL/timer/esp32c6_systimer.h>

/*
 * @file esp32c6.h
 * @brief Convenience initializers for ESP32-C6 device instances (stub).
 *
 * Peripheral base addresses from the ESP32-C6 TRM (System and Memory, peripheral
 * address table). Peripherals are mapped from 0x6000_0000.
 *   UART0:    0x6000_0000
 *   UART1:    0x6000_1000
 *   SYSTIMER: 0x6000_A000
 *   IO MUX:   0x6009_0000
 *   GPIO:     0x6009_1000
 */

/* --- Device macros --- */

#define WHAL_ESP32C6_UART0_BASE   0x60000000
#define WHAL_ESP32C6_UART0_DRIVER &whal_Esp32c6_Uart_Driver

#define WHAL_ESP32C6_UART1_BASE   0x60001000
#define WHAL_ESP32C6_UART1_DRIVER &whal_Esp32c6_Uart_Driver

#define WHAL_ESP32C6_SYSTIMER_BASE   0x6000A000
#define WHAL_ESP32C6_SYSTIMER_DRIVER &whal_Esp32c6_Systimer_Driver

#define WHAL_ESP32C6_IOMUX_BASE 0x60090000

#define WHAL_ESP32C6_GPIO_BASE   0x60091000
#define WHAL_ESP32C6_GPIO_DRIVER &whal_Esp32c6_Gpio_Driver

/* --- Peripheral clock gates (PCR, TRM chapter 8) --- */

#define WHAL_ESP32C6_UART0_GATE     \
    .confReg = 0x0000,              \
    .clkEnMsk = (1UL << 0),         \
    .rstEnMsk = (1UL << 1),         \
    .funcClkReg = 0x0004,           \
    .funcClkEnMsk = (1UL << 22)

#define WHAL_ESP32C6_IOMUX_GATE     \
    .confReg = 0x00E8,              \
    .clkEnMsk = (1UL << 0),         \
    .rstEnMsk = (1UL << 1),         \
    .funcClkReg = 0x00EC,           \
    .funcClkEnMsk = (1UL << 22)

#endif /* WHAL_ESP32C6_H */
