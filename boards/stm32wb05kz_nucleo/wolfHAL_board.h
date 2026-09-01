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
#include <wolfHAL/platform/st/stm32wb05kz.h>

extern whal_Uart g_whalUart;
extern whal_Spi g_whalSpi;
extern whal_I2c g_whalI2c;
#ifdef BOARD_WATCHDOG_IWDG
extern whal_Watchdog g_whalWatchdog;
#endif
extern whal_Crypto g_whalPka;

extern whal_Timeout g_whalTimeout;
extern volatile uint32_t g_tick;

enum {
    LED_PIN,
    BUTTON_PIN,
    UART_TX_PIN,
    UART_RX_PIN,
    SPI_SCK_PIN,
    SPI_MISO_PIN,
    SPI_MOSI_PIN,
    SPI_CS_PIN,
    I2C_SCL_PIN,
    I2C_SDA_PIN,
    PIN_COUNT,
};

#define BOARD_LED_PIN 0

/* Flash array on WB05KZ: 192 KB at 0x10040000, 2 KB pages = 96 pages.
 * Reserve the last page for test scratch. */
#define BOARD_FLASH_START_ADDR  WHAL_STM32WB05_FLASH_ARRAY_BASE
#define BOARD_FLASH_SIZE        WHAL_STM32WB05_FLASH_SIZE
#define BOARD_FLASH_SECTOR_SZ   0x800
#define BOARD_FLASH_TEST_ADDR   (BOARD_FLASH_START_ADDR + BOARD_FLASH_SIZE - BOARD_FLASH_SECTOR_SZ)

/* --- Single-instance device initializers ---
 * Each driver TU defines its singleton from these macros after #include
 * "wolfHAL_board.h". Names are WB0-prefixed; the alias header bridges them
 * to the WB-prefixed names that the leaf drivers consume.
 */

/* GPIO dev initializer — singleton defined in stm32wb0_gpio.c. */
#define WHAL_CFG_STM32WB0_GPIO_DEV { \
    .base = WHAL_STM32WB05_GPIO_BASE, \
    .cfg = (void *)&(const whal_Stm32wb0_Gpio_Cfg){ \
        .pinCfg = (const whal_Stm32wb0_Gpio_PinCfg[PIN_COUNT]){ \
            /* LD1 (blue) on PB1, output, push-pull, low speed, no pull */ \
            [LED_PIN] = WHAL_STM32WB0_GPIO_PIN( \
                WHAL_STM32WB0_GPIO_PORT_B, 1, WHAL_STM32WB0_GPIO_MODE_OUT, \
                WHAL_STM32WB0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB0_GPIO_SPEED_LOW, \
                WHAL_STM32WB0_GPIO_PULL_NONE, 0), \
            /* B1 user button on PA0, input, pull-up */ \
            [BUTTON_PIN] = WHAL_STM32WB0_GPIO_PIN( \
                WHAL_STM32WB0_GPIO_PORT_A, 0, WHAL_STM32WB0_GPIO_MODE_IN, \
                WHAL_STM32WB0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB0_GPIO_SPEED_LOW, \
                WHAL_STM32WB0_GPIO_PULL_UP, 0), \
            /* USART TX on PA1 AF2 (NUCLEO VCP TX) */ \
            [UART_TX_PIN] = WHAL_STM32WB0_GPIO_PIN( \
                WHAL_STM32WB0_GPIO_PORT_A, 1, WHAL_STM32WB0_GPIO_MODE_ALTFN, \
                WHAL_STM32WB0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB0_GPIO_SPEED_FAST, \
                WHAL_STM32WB0_GPIO_PULL_UP, 2), \
            /* USART RX on PB0 AF0 (NUCLEO VCP RX) */ \
            [UART_RX_PIN] = WHAL_STM32WB0_GPIO_PIN( \
                WHAL_STM32WB0_GPIO_PORT_B, 0, WHAL_STM32WB0_GPIO_MODE_ALTFN, \
                WHAL_STM32WB0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB0_GPIO_SPEED_FAST, \
                WHAL_STM32WB0_GPIO_PULL_UP, 0), \
            /* SPI3 SCK on PB3 AF4 */ \
            [SPI_SCK_PIN] = WHAL_STM32WB0_GPIO_PIN( \
                WHAL_STM32WB0_GPIO_PORT_B, 3, WHAL_STM32WB0_GPIO_MODE_ALTFN, \
                WHAL_STM32WB0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB0_GPIO_SPEED_FAST, \
                WHAL_STM32WB0_GPIO_PULL_NONE, 4), \
            /* SPI3 MISO on PA8 AF3 */ \
            [SPI_MISO_PIN] = WHAL_STM32WB0_GPIO_PIN( \
                WHAL_STM32WB0_GPIO_PORT_A, 8, WHAL_STM32WB0_GPIO_MODE_ALTFN, \
                WHAL_STM32WB0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB0_GPIO_SPEED_FAST, \
                WHAL_STM32WB0_GPIO_PULL_NONE, 3), \
            /* SPI3 MOSI on PA11 AF3 */ \
            [SPI_MOSI_PIN] = WHAL_STM32WB0_GPIO_PIN( \
                WHAL_STM32WB0_GPIO_PORT_A, 11, WHAL_STM32WB0_GPIO_MODE_ALTFN, \
                WHAL_STM32WB0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB0_GPIO_SPEED_FAST, \
                WHAL_STM32WB0_GPIO_PULL_NONE, 3), \
            /* SPI3 CS on PA9 as plain GPIO output (avoid SPI3_NSS AF) */ \
            [SPI_CS_PIN] = WHAL_STM32WB0_GPIO_PIN( \
                WHAL_STM32WB0_GPIO_PORT_A, 9, WHAL_STM32WB0_GPIO_MODE_OUT, \
                WHAL_STM32WB0_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB0_GPIO_SPEED_FAST, \
                WHAL_STM32WB0_GPIO_PULL_UP, 0), \
            /* I2C1 SCL on PB6 AF0, open-drain */ \
            [I2C_SCL_PIN] = WHAL_STM32WB0_GPIO_PIN( \
                WHAL_STM32WB0_GPIO_PORT_B, 6, WHAL_STM32WB0_GPIO_MODE_ALTFN, \
                WHAL_STM32WB0_GPIO_OUTTYPE_OPENDRAIN, WHAL_STM32WB0_GPIO_SPEED_FAST, \
                WHAL_STM32WB0_GPIO_PULL_UP, 0), \
            /* I2C1 SDA on PB7 AF0, open-drain */ \
            [I2C_SDA_PIN] = WHAL_STM32WB0_GPIO_PIN( \
                WHAL_STM32WB0_GPIO_PORT_B, 7, WHAL_STM32WB0_GPIO_MODE_ALTFN, \
                WHAL_STM32WB0_GPIO_OUTTYPE_OPENDRAIN, WHAL_STM32WB0_GPIO_SPEED_FAST, \
                WHAL_STM32WB0_GPIO_PULL_UP, 0), \
        }, \
        .pinCount = PIN_COUNT, \
    }, \
}

/* Flash dev initializer — singleton defined in stm32wb0_flash.c. */
#define WHAL_CFG_STM32WB0_FLASH_DEV { \
    .driver = WHAL_STM32WB05_FLASH_DRIVER, \
    .base   = WHAL_STM32WB05_FLASH_BASE, \
    .cfg    = (void *)&(const whal_Stm32wb0_Flash_Cfg){ \
        .startAddr = WHAL_STM32WB05_FLASH_ARRAY_BASE, \
        .size      = WHAL_STM32WB05_FLASH_SIZE, \
        .timeout   = &g_whalTimeout, \
    }, \
}

/* RNG dev initializer — singleton defined in stm32wb0_rng.c. */
#define WHAL_CFG_STM32WB0_RNG_DEV { \
    .base = WHAL_STM32WB05_RNG_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Stm32wb0_Rng_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

/* IWDG dev initializer — singleton defined in stm32wb_iwdg.c
 * (stm32wb0 alias points at the stm32wb leaf). */
#define WHAL_CFG_STM32WB0_IWDG_DEV { \
    .base = WHAL_STM32WB05_IWDG_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Stm32wb0_Iwdg_Cfg){ \
        .prescaler = WHAL_STM32WB0_IWDG_PR_32, \
        .reload    = 100, \
        .timeout   = &g_whalTimeout, \
    }, \
}

/* PKA dev initializer — singleton defined in stm32wb_pka.c
 * (stm32wb0 alias points at the stm32wb leaf). */
#define WHAL_CFG_STM32WB0_PKA_DEV { \
    .base   = WHAL_STM32WB05_PKA_BASE, \
    .driver = WHAL_STM32WB05_PKA_DRIVER, \
    .cfg    = (void *)&(const whal_Stm32wb0_Pka_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

/* SysTick dev initializer — singleton defined in systick.c. */
#define WHAL_CFG_SYSTICK_DEV { \
    .base = WHAL_CORTEX_M0PLUS_SYSTICK_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_SysTick_Cfg){ \
        .cyclesPerTick = 64000000 / 1000, \
        .clkSrc  = WHAL_SYSTICK_CLKSRC_SYSCLK, \
        .tickInt = WHAL_SYSTICK_TICKINT_ENABLED, \
    }, \
}

/* BOARD_*_DEV: how this board reaches each peripheral. */
#define BOARD_GPIO_DEV       WHAL_INTERNAL_DEV
#define BOARD_UART_DEV       (&g_whalUart)
#define BOARD_SPI_DEV        (&g_whalSpi)
#define BOARD_I2C_DEV        (&g_whalI2c)
#define BOARD_FLASH_DEV      ((whal_Flash *)&whal_Stm32wb0_Flash_Dev)
#define BOARD_RNG_DEV        WHAL_INTERNAL_DEV
#ifdef BOARD_WATCHDOG_IWDG
#define BOARD_WATCHDOG_DEV   (&g_whalWatchdog)
#endif
#define BOARD_PKA_DEV        (&g_whalPka)

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* WOLFHAL_BOARD_H */
