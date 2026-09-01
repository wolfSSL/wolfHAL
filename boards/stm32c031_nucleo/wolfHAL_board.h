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
#include <wolfHAL/platform/st/stm32c031xx.h>

extern whal_Uart g_whalUart;
extern whal_Spi g_whalSpi;

extern whal_Timeout g_whalTimeout;
extern volatile uint32_t g_tick;

enum {
    LED_PIN,
    UART_TX_PIN,
    UART_RX_PIN,
    SPI_SCK_PIN,
    SPI_MISO_PIN,
    SPI_MOSI_PIN,
    PIN_COUNT,
};

#define BOARD_LED_PIN 0

#define BOARD_FLASH_START_ADDR 0x08000000
#define BOARD_FLASH_SIZE       0x8000
#define BOARD_FLASH_TEST_ADDR  0x08007800
#define BOARD_FLASH_SECTOR_SZ  0x800

/* BOARD_*_DEV: how this board reaches each peripheral. */
#define BOARD_GPIO_DEV     WHAL_INTERNAL_DEV
#define BOARD_UART_DEV     (&g_whalUart)
#define BOARD_SPI_DEV      (&g_whalSpi)
#define BOARD_FLASH_DEV    ((whal_Flash *)&whal_Stm32c0_Flash_Dev)

/* Flash dev initializer — singleton defined in stm32c0_flash.c. */
#define WHAL_CFG_STM32C0_FLASH_DEV { \
    .driver = WHAL_STM32C031_FLASH_DRIVER, \
    .base   = WHAL_STM32C031_FLASH_BASE, \
    .cfg    = (void *)&(const whal_Stm32c0_Flash_Cfg){ \
        .startAddr = 0x08000000, \
        .size      = 0x8000, /* 32 KB */ \
        .timeout   = &g_whalTimeout, \
    }, \
}

/* GPIO dev initializer — singleton defined in stm32wb_gpio.c (shared driver). */
#define WHAL_CFG_STM32C0_GPIO_DEV { \
    .base = WHAL_STM32C031_GPIO_BASE, \
    .cfg = (void *)&(const whal_Stm32c0_Gpio_Cfg){ \
        .pinCfg = (const whal_Stm32c0_Gpio_PinCfg[PIN_COUNT]){ \
            /* LD4 Green LED on PA5 */ \
            [LED_PIN] = WHAL_STM32C0_GPIO_PIN( \
                WHAL_STM32C0_GPIO_PORT_A, 5, WHAL_STM32C0_GPIO_MODE_OUT, \
                WHAL_STM32C0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32C0_GPIO_SPEED_LOW, \
                WHAL_STM32C0_GPIO_PULL_NONE, 0), \
            /* USART2 TX on PA2, AF1 (ST-Link VCP) */ \
            [UART_TX_PIN] = WHAL_STM32C0_GPIO_PIN( \
                WHAL_STM32C0_GPIO_PORT_A, 2, WHAL_STM32C0_GPIO_MODE_ALTFN, \
                WHAL_STM32C0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32C0_GPIO_SPEED_FAST, \
                WHAL_STM32C0_GPIO_PULL_UP, 1), \
            /* USART2 RX on PA3, AF1 (ST-Link VCP) */ \
            [UART_RX_PIN] = WHAL_STM32C0_GPIO_PIN( \
                WHAL_STM32C0_GPIO_PORT_A, 3, WHAL_STM32C0_GPIO_MODE_ALTFN, \
                WHAL_STM32C0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32C0_GPIO_SPEED_FAST, \
                WHAL_STM32C0_GPIO_PULL_UP, 1), \
            /* SPI1 SCK on PA1, AF0 */ \
            [SPI_SCK_PIN] = WHAL_STM32C0_GPIO_PIN( \
                WHAL_STM32C0_GPIO_PORT_A, 1, WHAL_STM32C0_GPIO_MODE_ALTFN, \
                WHAL_STM32C0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32C0_GPIO_SPEED_FAST, \
                WHAL_STM32C0_GPIO_PULL_NONE, 0), \
            /* SPI1 MISO on PA6, AF0 */ \
            [SPI_MISO_PIN] = WHAL_STM32C0_GPIO_PIN( \
                WHAL_STM32C0_GPIO_PORT_A, 6, WHAL_STM32C0_GPIO_MODE_ALTFN, \
                WHAL_STM32C0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32C0_GPIO_SPEED_FAST, \
                WHAL_STM32C0_GPIO_PULL_NONE, 0), \
            /* SPI1 MOSI on PA7, AF0 */ \
            [SPI_MOSI_PIN] = WHAL_STM32C0_GPIO_PIN( \
                WHAL_STM32C0_GPIO_PORT_A, 7, WHAL_STM32C0_GPIO_MODE_ALTFN, \
                WHAL_STM32C0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32C0_GPIO_SPEED_FAST, \
                WHAL_STM32C0_GPIO_PULL_NONE, 0), \
        }, \
        .pinCount = PIN_COUNT, \
    }, \
}

/* SysTick dev initializer — singleton defined in systick.c. */
#define WHAL_CFG_SYSTICK_DEV { \
    .base = WHAL_CORTEX_M0PLUS_SYSTICK_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_SysTick_Cfg){ \
        .cyclesPerTick = 48000000 / 1000, \
        .clkSrc  = WHAL_SYSTICK_CLKSRC_SYSCLK, \
        .tickInt = WHAL_SYSTICK_TICKINT_ENABLED, \
    }, \
}

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* WOLFHAL_BOARD_H */
