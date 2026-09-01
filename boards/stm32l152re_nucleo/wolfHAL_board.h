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
#include <wolfHAL/platform/st/stm32l152re.h>
#include <wolfHAL/power/stm32l1_pwr.h>

extern whal_Uart g_whalUart;
extern whal_Spi g_whalSpi;
extern whal_I2c g_whalI2c;
extern whal_Watchdog g_whalWatchdog;

extern whal_Timeout g_whalTimeout;
extern volatile uint32_t g_tick;

enum {
    LED_PIN,
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

#define BOARD_FLASH_START_ADDR 0x08000000
#define BOARD_FLASH_SIZE       0x80000
#define BOARD_FLASH_TEST_ADDR  0x0807FF00
#define BOARD_FLASH_SECTOR_SZ  0x100

/* BOARD_*_DEV: how this board reaches each peripheral. */
#define BOARD_GPIO_DEV       WHAL_INTERNAL_DEV
#define BOARD_UART_DEV       (&g_whalUart)
#define BOARD_SPI_DEV        (&g_whalSpi)
#define BOARD_I2C_DEV        (&g_whalI2c)
#define BOARD_FLASH_DEV      ((whal_Flash *)&whal_Stm32l1_Flash_Dev)
#define BOARD_WATCHDOG_DEV   (&g_whalWatchdog)

/* WWDG dev initializer — singleton defined in stm32f0_wwdg.c (stm32l1 is
 * an include alias). Compiled unconditionally because the .c always
 * references it. */
#define WHAL_CFG_STM32L1_WWDG_DEV { \
    .base = WHAL_STM32L152_WWDG_BASE, \
    .cfg  = (void *)&(const whal_Stm32l1_Wwdg_Cfg){ \
        .prescaler = 3, \
        .window    = 0x7F, \
        .counter   = 0x7F, \
    }, \
}

/* IWDG dev initializer — singleton defined in stm32wb_iwdg.c (stm32l1 is
 * an include alias). */
#define WHAL_CFG_STM32L1_IWDG_DEV { \
    .base = WHAL_STM32L152_IWDG_BASE, \
    .cfg  = (void *)&(const whal_Stm32l1_Iwdg_Cfg){ \
        .prescaler = WHAL_STM32L1_IWDG_PR_64, \
        .reload    = 500, \
        .timeout   = &g_whalTimeout, \
    }, \
}

/* Flash dev initializer — singleton defined in stm32l1_flash.c. */
#define WHAL_CFG_STM32L1_FLASH_DEV { \
    .driver = WHAL_STM32L152_FLASH_DRIVER, \
    .base   = WHAL_STM32L152_FLASH_BASE, \
    .cfg    = (void *)&(const whal_Stm32l1_Flash_Cfg){ \
        .startAddr = 0x08000000, \
        .size      = 0x80000, \
        .timeout   = &g_whalTimeout, \
    }, \
}

/* GPIO dev initializer — singleton defined in driver TU. */
#define WHAL_CFG_STM32L1_GPIO_DEV { \
    .base = WHAL_STM32L152_GPIO_BASE, \
    .cfg = (void *)&(const whal_Stm32l1_Gpio_Cfg){ \
        .pinCfg = (const whal_Stm32l1_Gpio_PinCfg[PIN_COUNT]){ \
            /* LD2 Green LED on PA5 (per UM1724, NUCLEO-L152RE) */ \
            [LED_PIN] = WHAL_STM32L1_GPIO_PIN( \
                WHAL_STM32L1_GPIO_PORT_A, 5, WHAL_STM32L1_GPIO_MODE_OUT, \
                WHAL_STM32L1_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32L1_GPIO_SPEED_LOW, \
                WHAL_STM32L1_GPIO_PULL_NONE, 0), \
            /* USART2 TX on PA2, AF7 */ \
            [UART_TX_PIN] = WHAL_STM32L1_GPIO_PIN( \
                WHAL_STM32L1_GPIO_PORT_A, 2, WHAL_STM32L1_GPIO_MODE_ALTFN, \
                WHAL_STM32L1_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32L1_GPIO_SPEED_FAST, \
                WHAL_STM32L1_GPIO_PULL_UP, 7), \
            /* USART2 RX on PA3, AF7 */ \
            [UART_RX_PIN] = WHAL_STM32L1_GPIO_PIN( \
                WHAL_STM32L1_GPIO_PORT_A, 3, WHAL_STM32L1_GPIO_MODE_ALTFN, \
                WHAL_STM32L1_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32L1_GPIO_SPEED_FAST, \
                WHAL_STM32L1_GPIO_PULL_UP, 7), \
            /* SPI3 SCK on PB3, AF6 (avoids LD2 conflict on PA5) */ \
            [SPI_SCK_PIN] = WHAL_STM32L1_GPIO_PIN( \
                WHAL_STM32L1_GPIO_PORT_B, 3, WHAL_STM32L1_GPIO_MODE_ALTFN, \
                WHAL_STM32L1_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32L1_GPIO_SPEED_FAST, \
                WHAL_STM32L1_GPIO_PULL_NONE, 6), \
            /* SPI3 MISO on PB4, AF6 */ \
            [SPI_MISO_PIN] = WHAL_STM32L1_GPIO_PIN( \
                WHAL_STM32L1_GPIO_PORT_B, 4, WHAL_STM32L1_GPIO_MODE_ALTFN, \
                WHAL_STM32L1_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32L1_GPIO_SPEED_FAST, \
                WHAL_STM32L1_GPIO_PULL_NONE, 6), \
            /* SPI3 MOSI on PB5, AF6 */ \
            [SPI_MOSI_PIN] = WHAL_STM32L1_GPIO_PIN( \
                WHAL_STM32L1_GPIO_PORT_B, 5, WHAL_STM32L1_GPIO_MODE_ALTFN, \
                WHAL_STM32L1_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32L1_GPIO_SPEED_FAST, \
                WHAL_STM32L1_GPIO_PULL_NONE, 6), \
            /* SPI CS on PB12, output, push-pull */ \
            [SPI_CS_PIN] = WHAL_STM32L1_GPIO_PIN( \
                WHAL_STM32L1_GPIO_PORT_B, 12, WHAL_STM32L1_GPIO_MODE_OUT, \
                WHAL_STM32L1_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32L1_GPIO_SPEED_FAST, \
                WHAL_STM32L1_GPIO_PULL_UP, 0), \
            /* I2C1 SCL on PB8, AF4, open-drain */ \
            [I2C_SCL_PIN] = WHAL_STM32L1_GPIO_PIN( \
                WHAL_STM32L1_GPIO_PORT_B, 8, WHAL_STM32L1_GPIO_MODE_ALTFN, \
                WHAL_STM32L1_GPIO_OUTTYPE_OPENDRAIN, WHAL_STM32L1_GPIO_SPEED_FAST, \
                WHAL_STM32L1_GPIO_PULL_UP, 4), \
            /* I2C1 SDA on PB9, AF4, open-drain */ \
            [I2C_SDA_PIN] = WHAL_STM32L1_GPIO_PIN( \
                WHAL_STM32L1_GPIO_PORT_B, 9, WHAL_STM32L1_GPIO_MODE_ALTFN, \
                WHAL_STM32L1_GPIO_OUTTYPE_OPENDRAIN, WHAL_STM32L1_GPIO_SPEED_FAST, \
                WHAL_STM32L1_GPIO_PULL_UP, 4), \
        }, \
        .pinCount = PIN_COUNT, \
    }, \
}

/* SysTick dev initializer — singleton defined in systick.c. */
#define WHAL_CFG_SYSTICK_DEV { \
    .base = WHAL_CORTEX_M3_SYSTICK_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_SysTick_Cfg){ \
        .cyclesPerTick = 32000000 / 1000, \
        .clkSrc  = WHAL_SYSTICK_CLKSRC_SYSCLK, \
        .tickInt = WHAL_SYSTICK_TICKINT_ENABLED, \
    }, \
}

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* WOLFHAL_BOARD_H */
