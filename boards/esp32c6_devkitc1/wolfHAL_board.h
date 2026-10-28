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
extern whal_Timeout g_whalTimeout;
extern volatile uint32_t g_tick;

#define BOARD_LED_PIN 0

/* BOARD_*_DEV: how this board reaches each peripheral. */
#define BOARD_GPIO_DEV  WHAL_INTERNAL_DEV
#define BOARD_UART_DEV  (&g_whalUart)
#define BOARD_TIMER_DEV (&g_whalTimer)

/* GPIO dev initializer: GPIO8 (BOARD_LED_PIN) as a simple push-pull output. */
#define WHAL_CFG_ESP32C6_GPIO_DEV { \
    .base = WHAL_ESP32C6_GPIO_BASE, \
    /* .driver: direct API mapping */ \
    .cfg = (void *)&(const whal_Esp32c6_Gpio_Cfg){ \
        .pinCfg = (const whal_Esp32c6_Gpio_PinCfg[]){ \
            { .pin = 8, \
              .outputEnable = 1, \
              .gpioCfg  = WHAL_ESP32C6_GPIO_PINREG(0, GPIO_INT_DISABLE, 0, 0), \
              .ioMuxCfg = WHAL_ESP32C6_GPIO_IOMUX(IO_MUX_FUNC_GPIO, 0, 0, 2) }, \
        }, \
        .pinCount = 1, \
        .outputCfg = (const whal_Esp32c6_Gpio_OutputCfg[]){ \
            { .gpioNum = 8, .cfg = WHAL_ESP32C6_GPIO_OUT(GPIO_SIG_GPIO) }, \
        }, \
        .outputCount = 1, \
    }, \
}

/* UART dev initializer. */
#define WHAL_CFG_ESP32C6_UART_DEV { \
    .base = WHAL_ESP32C6_UART0_BASE, \
    .driver = WHAL_ESP32C6_UART0_DRIVER, \
    .cfg = &(whal_Esp32c6_Uart_Cfg){ \
        .baud = 115200, \
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

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* WOLFHAL_BOARD_H */
