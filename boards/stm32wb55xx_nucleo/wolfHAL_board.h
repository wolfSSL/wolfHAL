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
#include <wolfHAL/platform/st/stm32wb55xx.h>

extern whal_Uart g_whalUart;
extern whal_Spi g_whalSpi;
extern whal_Crypto g_whalCrypto;
extern whal_I2c g_whalI2c;
extern whal_Pwm g_whalPwm;

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
    LPTIM1_OUT_PIN,
    PIN_COUNT,
};

#define BOARD_LED_PIN             0
#define BOARD_FLASH_START_ADDR    0x08000000
#define BOARD_FLASH_SIZE          0x80000  /* 512 KB (upper half reserved for BLE stack) */
#define BOARD_FLASH_TEST_ADDR     0x0807F000
#define BOARD_FLASH_SECTOR_SZ     0x1000

/* BOARD_*_DEV: how this board reaches each peripheral. WHAL_INTERNAL_DEV for
 * single-instance drivers (driver ignores the pointer); &g_whal<X> for
 * drivers still using vtable dispatch / pointer-based path. */
#define BOARD_GPIO_DEV       WHAL_INTERNAL_DEV
#define BOARD_UART_DEV       (&g_whalUart)
#define BOARD_SPI_DEV        (&g_whalSpi)
#define BOARD_I2C_DEV        (&g_whalI2c)
#define BOARD_PWM_DEV        (&g_whalPwm)
#define BOARD_FLASH_DEV      ((whal_Flash *)&whal_Stm32wb_Flash_Dev)
#define BOARD_WATCHDOG_DEV   WHAL_INTERNAL_DEV
#define BOARD_RNG_DEV        WHAL_INTERNAL_DEV
#define BOARD_AES_ECB_DEV    WHAL_INTERNAL_DEV
#define BOARD_AES_CBC_DEV    WHAL_INTERNAL_DEV
#define BOARD_AES_CTR_DEV    WHAL_INTERNAL_DEV
#define BOARD_AES_GCM_DEV    WHAL_INTERNAL_DEV
#define BOARD_AES_GMAC_DEV   WHAL_INTERNAL_DEV
#define BOARD_AES_CCM_DEV    WHAL_INTERNAL_DEV

/* GPIO dev initializer — single-instance device defined in stm32wb_gpio.c. */
#define WHAL_CFG_STM32WB_GPIO_DEV { \
    .base = WHAL_STM32WB55_GPIO_BASE, \
    .cfg  = (void *)&(const whal_Stm32wb_Gpio_Cfg){ \
        .pinCfg = (const whal_Stm32wb_Gpio_PinCfg[PIN_COUNT]){ \
            [LED_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_B, 5, WHAL_STM32WB_GPIO_MODE_OUT, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_LOW, \
                WHAL_STM32WB_GPIO_PULL_UP, 0), \
            [UART_TX_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_B, 6, WHAL_STM32WB_GPIO_MODE_ALTFN, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_FAST, \
                WHAL_STM32WB_GPIO_PULL_UP, 7), \
            [UART_RX_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_B, 7, WHAL_STM32WB_GPIO_MODE_ALTFN, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_FAST, \
                WHAL_STM32WB_GPIO_PULL_UP, 7), \
            [SPI_SCK_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_A, 5, WHAL_STM32WB_GPIO_MODE_ALTFN, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_FAST, \
                WHAL_STM32WB_GPIO_PULL_NONE, 5), \
            [SPI_MISO_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_A, 6, WHAL_STM32WB_GPIO_MODE_ALTFN, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_FAST, \
                WHAL_STM32WB_GPIO_PULL_NONE, 5), \
            [SPI_MOSI_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_A, 7, WHAL_STM32WB_GPIO_MODE_ALTFN, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_FAST, \
                WHAL_STM32WB_GPIO_PULL_NONE, 5), \
            [SPI_CS_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_A, 4, WHAL_STM32WB_GPIO_MODE_OUT, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_FAST, \
                WHAL_STM32WB_GPIO_PULL_UP, 0), \
            [I2C_SCL_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_B, 8, WHAL_STM32WB_GPIO_MODE_ALTFN, \
                WHAL_STM32WB_GPIO_OUTTYPE_OPENDRAIN, WHAL_STM32WB_GPIO_SPEED_FAST, \
                WHAL_STM32WB_GPIO_PULL_UP, 4), \
            [I2C_SDA_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_B, 9, WHAL_STM32WB_GPIO_MODE_ALTFN, \
                WHAL_STM32WB_GPIO_OUTTYPE_OPENDRAIN, WHAL_STM32WB_GPIO_SPEED_FAST, \
                WHAL_STM32WB_GPIO_PULL_UP, 4), \
            [LPTIM1_OUT_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_B, 2, WHAL_STM32WB_GPIO_MODE_ALTFN, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_LOW, \
                WHAL_STM32WB_GPIO_PULL_NONE, 1), \
        }, \
        .pinCount = PIN_COUNT, \
    }, \
}

/* AES crypto + mode dev initializers — single-instance devices defined in stm32wb_aes.c.
 * Mutable GCM/CCM state buffers (g_stm32wbAesGcm/CcmDevState) are static in
 * the driver TU. */
#define WHAL_CFG_STM32WB_AES_DEV { \
    .base = WHAL_STM32WB55_AES1_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Stm32wb_Aes_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

#define WHAL_CFG_STM32WB_AES_ECB_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
    /* .driver: direct API mapping */ \
}

#define WHAL_CFG_STM32WB_AES_CBC_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
    /* .driver: direct API mapping */ \
}

#define WHAL_CFG_STM32WB_AES_CTR_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
    /* .driver: direct API mapping */ \
}

#define WHAL_CFG_STM32WB_AES_GCM_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
    /* .driver: direct API mapping */ \
    .state  = &g_stm32wbAesGcmDevState, \
}

#define WHAL_CFG_STM32WB_AES_GMAC_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
    /* .driver: direct API mapping */ \
}

#define WHAL_CFG_STM32WB_AES_CCM_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
    /* .driver: direct API mapping */ \
    .state  = &g_stm32wbAesCcmDevState, \
}

/* PKA peripheral dev initializer — single-instance device defined in
 * stm32wb_pka.c. PKA uses vtable dispatch for whal_Crypto_Init (AES
 * already claims the direct mapping at the Crypto type), so .driver is
 * set. The math primitives (whal_Pka_*) are direct functions and don't
 * need a device wrapper. */
#define WHAL_CFG_STM32WB_PKA_DEV { \
    .base   = WHAL_STM32WB55_PKA_BASE, \
    .driver = WHAL_STM32WB55_PKA_DRIVER, \
    .cfg    = (void *)&(const whal_Stm32wb_Pka_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

/* Flash dev initializer — the single-instance device itself is defined in stm32wb_flash.c
 * (which #includes this header). BOARD_FLASH_DEV takes its address so
 * whal_Flash_* can dispatch via .driver alongside coexisting flash drivers
 * (e.g. SPI NOR W25Q64). */
#define WHAL_CFG_STM32WB_FLASH_DEV { \
    .driver = WHAL_STM32WB55_FLASH_DRIVER, \
    .base   = WHAL_STM32WB55_FLASH_BASE, \
    .cfg    = (void *)&(const whal_Stm32wb_Flash_Cfg){ \
        .timeout   = &g_whalTimeout, \
        .startAddr = 0x08000000, \
        .size      = 0x80000, /* 512 KB (upper half reserved for BLE stack) */ \
    }, \
}

/* IWDG dev initializer — single-instance device defined in stm32wb_iwdg.c. */
#define WHAL_CFG_STM32WB_IWDG_DEV { \
    .base = WHAL_STM32WB55_IWDG_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Stm32wb_Iwdg_Cfg){ \
        .prescaler = WHAL_STM32WB_IWDG_PR_32, \
        .reload    = 100, \
        .timeout   = &g_whalTimeout, \
    }, \
}

/* WWDG dev initializer — single-instance device defined in stm32wb_wwdg.c. */
#define WHAL_CFG_STM32WB_WWDG_DEV { \
    .base = WHAL_STM32WB55_WWDG_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Stm32wb_Wwdg_Cfg){ \
        .prescaler = WHAL_STM32WB_WWDG_TB_128, \
        .window    = 0x7F, \
        .counter   = 0x7F, \
    }, \
}

/* RNG dev initializer — single-instance device defined in stm32wb_rng.c. */
#define WHAL_CFG_STM32WB_RNG_DEV { \
    .base = WHAL_STM32WB55_RNG_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Stm32wb_Rng_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

/* NVIC dev initializer — single-instance device defined in cortex_m4_nvic.c. */
#define WHAL_CFG_NVIC_DEV { \
    .base = WHAL_CORTEX_M4_NVIC_BASE, \
    /* .driver: direct API mapping */ \
}

/* SysTick dev initializer — single-instance device defined in systick.c. */
#define WHAL_CFG_SYSTICK_DEV { \
    .base = WHAL_CORTEX_M4_SYSTICK_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_SysTick_Cfg){ \
        .cyclesPerTick = 64000000 / 1000, \
        .clkSrc  = WHAL_SYSTICK_CLKSRC_SYSCLK, \
        .tickInt = WHAL_SYSTICK_TICKINT_ENABLED, \
    }, \
}

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* WOLFHAL_BOARD_H */
