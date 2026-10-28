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
#include <wolfHAL/platform/esp/esp32c6.h>

extern whal_Uart g_whalUart;
extern whal_Timer g_whalTimer;
extern whal_Watchdog g_whalTimg0Wdt;
extern whal_Timeout g_whalTimeout;
extern volatile uint32_t g_tick;

#define BOARD_LED_PIN 0

/* CPU interrupt line carrying the SYSTIMER tick; ivt.c slot 1 must match. */
#define BOARD_SYSTIMER_CPU_INT 1

/* BOARD_*_DEV: how this board reaches each peripheral. */
#define BOARD_GPIO_DEV  WHAL_INTERNAL_DEV
#define BOARD_UART_DEV  (&g_whalUart)
#define BOARD_TIMER_DEV (&g_whalTimer)
#define BOARD_RTC_WDT_DEV ((whal_Watchdog *)&whal_Esp32c6_RtcWdt_Dev)

/* GPIO dev initializer: GPIO15 (BOARD_LED_PIN, active-low user LED) as a
 * push-pull output, GPIO16/GPIO17 (D6/D7) as UART0 TX/RX through the IO MUX. */
#define WHAL_CFG_ESP32C6_GPIO_DEV { \
    .base = WHAL_ESP32C6_GPIO_BASE, \
    /* .driver: direct API mapping */ \
    .cfg = (void *)&(const whal_Esp32c6_Gpio_Cfg){ \
        .pinCfg = (const whal_Esp32c6_Gpio_PinCfg[]){ \
            { .pin = 15, \
              .outputEnable = 1, \
              .gpioCfg  = WHAL_ESP32C6_GPIO_PINREG(0, GPIO_INT_DISABLE, 0, 0), \
              .ioMuxCfg = WHAL_ESP32C6_GPIO_IOMUX(IO_MUX_FUNC_GPIO, 0, 0, 2) }, \
            { .pin = 16, \
              .gpioCfg  = WHAL_ESP32C6_GPIO_PINREG(0, GPIO_INT_DISABLE, 0, 0), \
              .ioMuxCfg = WHAL_ESP32C6_GPIO_IOMUX(WHAL_ESP32C6_IOMUX_FUNC_U0TXD, 0, 0, 2) }, \
            { .pin = 17, \
              .gpioCfg  = WHAL_ESP32C6_GPIO_PINREG(0, GPIO_INT_DISABLE, 0, 0), \
              .ioMuxCfg = WHAL_ESP32C6_GPIO_IOMUX(WHAL_ESP32C6_IOMUX_FUNC_U0RXD, 1, \
                                                  IO_MUX_FUN_WPU_Msk, 2) }, \
        }, \
        .pinCount = 3, \
        .outputCfg = (const whal_Esp32c6_Gpio_OutputCfg[]){ \
            { .gpioNum = 15, .cfg = WHAL_ESP32C6_GPIO_OUT(GPIO_SIG_GPIO) }, \
        }, \
        .outputCount = 1, \
    }, \
}

/* UART dev initializer. */
#define WHAL_CFG_ESP32C6_UART_DEV { \
    .base = WHAL_ESP32C6_UART0_BASE, \
    .driver = WHAL_ESP32C6_UART0_DRIVER, \
    .cfg = &(whal_Esp32c6_Uart_Cfg){ \
        .clkDiv = WHAL_ESP32C6_UART_CLKDIV(115200, 40000000), \
        .timeout = &g_whalTimeout, \
    }, \
}

/* SYSTIMER dev initializer: 1 ms periodic tick on UNIT0/COMP0 (16 MHz counter). */
#define WHAL_CFG_ESP32C6_SYSTIMER_DEV { \
    .base = WHAL_ESP32C6_SYSTIMER_BASE, \
    .driver = WHAL_ESP32C6_SYSTIMER_DRIVER, \
    .cfg = &(whal_Esp32c6_Systimer_Cfg){ \
        .mode = WHAL_ESP32C6_SYSTIMER_PERIODIC, \
        .unit = WHAL_ESP32C6_SYSTIMER_UNIT0, \
        .comp = WHAL_ESP32C6_SYSTIMER_COMP0, \
        .compValue = 0, \
        .period = 16000, \
    }, \
}

/* Interrupt controller dev initializer: single-instance device defined in
 * esp32c6_intc.c. */
#define WHAL_CFG_ESP32C6_INTC_DEV { \
    .base = WHAL_ESP32C6_INTMTX_BASE, \
    /* .driver: direct API mapping */ \
    .cfg = (void *)&(const whal_Esp32c6_Intc_DevCfg){ \
        .plicMxBase = WHAL_ESP32C6_PLIC_MX_BASE, \
    }, \
}

/* RTC watchdog dev initializer: single-instance device defined in
 * esp32c6_rtc_wdt.c. stg0Hold is about 1 s of the 136 kHz RTC slow clock. */
#define WHAL_CFG_ESP32C6_RTC_WDT_DEV { \
    .base = WHAL_ESP32C6_RTC_WDT_BASE, \
    .driver = WHAL_ESP32C6_RTC_WDT_DRIVER, \
    .cfg = (void *)&(const whal_Esp32c6_RtcWdt_Cfg){ \
        .stg0Hold = 136000, \
    }, \
}

/* TIMG0 watchdog dev initializer: 40 MHz XTAL / 40000 = 1 kHz, 1 s timeout. */
#define WHAL_CFG_ESP32C6_TIMG_WDT_DEV { \
    .base = WHAL_ESP32C6_TIMG0_BASE, \
    .driver = WHAL_ESP32C6_TIMG_WDT_DRIVER, \
    .cfg = (void *)&(const whal_Esp32c6_TimgWdt_Cfg){ \
        .prescale = 40000, \
        .stg0Hold = 1000, \
    }, \
}

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* WOLFHAL_BOARD_H */
