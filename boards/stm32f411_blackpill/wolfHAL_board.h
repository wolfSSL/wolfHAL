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
#include <wolfHAL/platform/st/stm32f411xx.h>

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

/* Flash test address: last sector (sector 7, 128KB at 0x08060000) */
#define BOARD_FLASH_TEST_ADDR 0x08060000
#define BOARD_FLASH_SECTOR_SZ 0x20000
#define BOARD_FLASH_WRITE_SZ  4

/* BOARD_*_DEV: how this board reaches each peripheral. */
#define BOARD_GPIO_DEV     WHAL_INTERNAL_DEV
#define BOARD_UART_DEV     (&g_whalUart)
#define BOARD_SPI_DEV      (&g_whalSpi)
#define BOARD_FLASH_DEV    ((whal_Flash *)&whal_Stm32f4_Flash_Dev)

/* Flash sector layout (defined in wolfHAL_board.c), referenced by the flash
 * singleton's cfg below. */
#define FLASH_SECTOR_COUNT 8
extern const whal_Stm32f4_Flash_Sector g_flashSectors[FLASH_SECTOR_COUNT];

/* Flash dev initializer — singleton defined in stm32f4_flash.c. */
#define WHAL_CFG_STM32F4_FLASH_DEV { \
    .driver = WHAL_STM32F411_FLASH_DRIVER, \
    .base   = WHAL_STM32F411_FLASH_BASE, \
    .cfg    = (void *)&(const whal_Stm32f4_Flash_Cfg){ \
        .startAddr   = 0x08000000, \
        .size        = 0x80000, /* 512 KB */ \
        .sectors     = g_flashSectors, \
        .sectorCount = FLASH_SECTOR_COUNT, \
        .timeout     = &g_whalTimeout, \
    }, \
}

/* GPIO dev initializer — singleton defined in driver TU. */
#define WHAL_CFG_STM32F4_GPIO_DEV { \
    .base = WHAL_STM32F411_GPIO_BASE, \
    .cfg = (void *)&(const whal_Stm32f4_Gpio_Cfg){ \
        .pinCfg = (const whal_Stm32f4_Gpio_PinCfg[PIN_COUNT]){ \
            /* LED on PC13 (active low) */ \
            [LED_PIN] = WHAL_STM32F4_GPIO_PIN( \
                WHAL_STM32F4_GPIO_PORT_C, 13, WHAL_STM32F4_GPIO_MODE_OUT, \
                WHAL_STM32F4_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32F4_GPIO_SPEED_LOW, \
                WHAL_STM32F4_GPIO_PULL_NONE, 0), \
            /* USART2 TX on PA2 (AF7) */ \
            [UART_TX_PIN] = WHAL_STM32F4_GPIO_PIN( \
                WHAL_STM32F4_GPIO_PORT_A, 2, WHAL_STM32F4_GPIO_MODE_ALTFN, \
                WHAL_STM32F4_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32F4_GPIO_SPEED_FAST, \
                WHAL_STM32F4_GPIO_PULL_UP, 7), \
            /* USART2 RX on PA3 (AF7) */ \
            [UART_RX_PIN] = WHAL_STM32F4_GPIO_PIN( \
                WHAL_STM32F4_GPIO_PORT_A, 3, WHAL_STM32F4_GPIO_MODE_ALTFN, \
                WHAL_STM32F4_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32F4_GPIO_SPEED_FAST, \
                WHAL_STM32F4_GPIO_PULL_UP, 7), \
            /* SPI1 SCK on PA5 (AF5) */ \
            [SPI_SCK_PIN] = WHAL_STM32F4_GPIO_PIN( \
                WHAL_STM32F4_GPIO_PORT_A, 5, WHAL_STM32F4_GPIO_MODE_ALTFN, \
                WHAL_STM32F4_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32F4_GPIO_SPEED_FAST, \
                WHAL_STM32F4_GPIO_PULL_NONE, 5), \
            /* SPI1 MISO on PA6 (AF5) */ \
            [SPI_MISO_PIN] = WHAL_STM32F4_GPIO_PIN( \
                WHAL_STM32F4_GPIO_PORT_A, 6, WHAL_STM32F4_GPIO_MODE_ALTFN, \
                WHAL_STM32F4_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32F4_GPIO_SPEED_FAST, \
                WHAL_STM32F4_GPIO_PULL_NONE, 5), \
            /* SPI1 MOSI on PA7 (AF5) */ \
            [SPI_MOSI_PIN] = WHAL_STM32F4_GPIO_PIN( \
                WHAL_STM32F4_GPIO_PORT_A, 7, WHAL_STM32F4_GPIO_MODE_ALTFN, \
                WHAL_STM32F4_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32F4_GPIO_SPEED_FAST, \
                WHAL_STM32F4_GPIO_PULL_NONE, 5), \
        }, \
        .pinCount = PIN_COUNT, \
    }, \
}

/* SysTick dev initializer — singleton defined in systick.c. */
#define WHAL_CFG_SYSTICK_DEV { \
    .base = WHAL_CORTEX_M4_SYSTICK_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_SysTick_Cfg){ \
        .cyclesPerTick = 100000000 / 1000, \
        .clkSrc  = WHAL_SYSTICK_CLKSRC_SYSCLK, \
        .tickInt = WHAL_SYSTICK_TICKINT_ENABLED, \
    }, \
}

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* WOLFHAL_BOARD_H */
