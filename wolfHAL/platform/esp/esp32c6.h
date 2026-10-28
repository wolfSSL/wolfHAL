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
#include <wolfHAL/irq/esp32c6_intc.h>
#include <wolfHAL/watchdog/esp32c6_rtc_wdt.h>
#include <wolfHAL/watchdog/esp32c6_timg_wdt.h>

/*
 * @file esp32c6.h
 * @brief Convenience initializers for ESP32-C6 device instances.
 *
 * Peripheral base addresses from the ESP32-C6 TRM (System and Memory, peripheral
 * address table). Peripherals are mapped from 0x6000_0000.
 *   UART0:    0x6000_0000
 *   UART1:    0x6000_1000
 *   TIMG0:    0x6000_8000
 *   TIMG1:    0x6000_9000
 *   SYSTIMER: 0x6000_A000
 *   INTMTX:   0x6001_0000
 *   IO MUX:   0x6009_0000
 *   GPIO:     0x6009_1000
 *   RTC_WDT:  0x600B_1C00
 *   PLIC MX:  0x2000_1000
 */

/* --- Device macros --- */

#define WHAL_ESP32C6_UART0_BASE   0x60000000
#define WHAL_ESP32C6_UART0_DRIVER &whal_Esp32c6_Uart_Driver

#define WHAL_ESP32C6_UART1_BASE   0x60001000
#define WHAL_ESP32C6_UART1_DRIVER &whal_Esp32c6_Uart_Driver

#define WHAL_ESP32C6_TIMG0_BASE      0x60008000
#define WHAL_ESP32C6_TIMG1_BASE      0x60009000
#define WHAL_ESP32C6_TIMG_WDT_DRIVER &whal_Esp32c6_TimgWdt_Driver

#define WHAL_ESP32C6_SYSTIMER_BASE   0x6000A000
#define WHAL_ESP32C6_SYSTIMER_DRIVER &whal_Esp32c6_Systimer_Driver

#define WHAL_ESP32C6_INTMTX_BASE   0x60010000
#define WHAL_ESP32C6_PLIC_MX_BASE  0x20001000
#define WHAL_ESP32C6_INTC_DRIVER   &whal_Esp32c6_Intc_Driver

#define WHAL_ESP32C6_IOMUX_BASE 0x60090000

/* IO MUX function routing UART0 directly on GPIO16 (TX) and GPIO17 (RX). */
#define WHAL_ESP32C6_IOMUX_FUNC_U0TXD 0
#define WHAL_ESP32C6_IOMUX_FUNC_U0RXD 0

#define WHAL_ESP32C6_GPIO_BASE   0x60091000
#define WHAL_ESP32C6_GPIO_DRIVER &whal_Esp32c6_Gpio_Driver

#define WHAL_ESP32C6_RTC_WDT_BASE   0x600B1C00
#define WHAL_ESP32C6_RTC_WDT_DRIVER &whal_Esp32c6_RtcWdt_Driver

/* --- Interrupt matrix sources (TRM Table 10.3-1) --- */

#define WHAL_ESP32C6_INTSRC_UART0             43
#define WHAL_ESP32C6_INTSRC_SYSTIMER_TARGET0  57
#define WHAL_ESP32C6_INTSRC_SYSTIMER_TARGET1  58
#define WHAL_ESP32C6_INTSRC_SYSTIMER_TARGET2  59

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
