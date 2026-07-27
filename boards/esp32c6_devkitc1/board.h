/* board.h
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

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/platform/esp/esp32c6.h>

extern whal_Gpio g_whalGpio;
extern whal_Uart g_whalUart;
extern whal_Timer g_whalTimer;
extern whal_Timeout g_whalTimeout;
extern volatile uint32_t g_tick;

#define BOARD_LED_PIN 0

/* BOARD_*_DEV: how this board reaches each peripheral. */
#define BOARD_GPIO_DEV  (&g_whalGpio)
#define BOARD_UART_DEV  (&g_whalUart)
#define BOARD_TIMER_DEV (&g_whalTimer)

/* GPIO dev initializer. Pin table left empty. */
#define WHAL_CFG_ESP32C6_GPIO_DEV { \
    .base = WHAL_ESP32C6_GPIO_BASE, \
    .driver = WHAL_ESP32C6_GPIO_DRIVER, \
    .cfg = (void *)&(const whal_Esp32c6_Gpio_Cfg){ \
        .pinCfg = NULL, \
        .pinCount = 0, \
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

/* SYSTIMER dev initializer. */
#define WHAL_CFG_ESP32C6_SYSTIMER_DEV { \
    .base = WHAL_ESP32C6_SYSTIMER_BASE, \
    .driver = WHAL_ESP32C6_SYSTIMER_DRIVER, \
    .cfg = &(whal_Esp32c6_Systimer_Cfg){ \
        .period = 1000, \
    }, \
}

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* BOARD_H */
