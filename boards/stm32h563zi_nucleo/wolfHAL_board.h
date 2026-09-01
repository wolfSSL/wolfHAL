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
#include <wolfHAL/platform/st/stm32h563xx.h>
#include <wolfHAL/eth_phy/lan8742a_eth_phy.h>

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
    SPI_CS_PIN,
    ETH_RMII_REF_CLK_PIN,
    ETH_RMII_MDIO_PIN,
    ETH_RMII_MDC_PIN,
    ETH_RMII_CRS_DV_PIN,
    ETH_RMII_RXD0_PIN,
    ETH_RMII_RXD1_PIN,
    ETH_RMII_TX_EN_PIN,
    ETH_RMII_TXD0_PIN,
    ETH_RMII_TXD1_PIN,
    PIN_COUNT,
};

#define BOARD_LED_PIN 0

/* Flash test address: last sector of bank 2 (safe area away from firmware) */
#define BOARD_FLASH_START_ADDR 0x08000000
#define BOARD_FLASH_SIZE       0x200000
#define BOARD_FLASH_TEST_ADDR  0x081FE000
#define BOARD_FLASH_SECTOR_SZ  0x2000

/* Ethernet PHY: LAN8742A on MDIO address 0 */
#define BOARD_ETH_PHY_ADDR 0
#define BOARD_ETH_PHY_ID1  0x0007
#define BOARD_ETH_PHY_ID2  0xC131

/* BOARD_*_DEV: how this board reaches each peripheral. WHAL_INTERNAL_DEV for
 * single-instance drivers; &g_whal<X> for pointer-based drivers. */
#define BOARD_GPIO_DEV     WHAL_INTERNAL_DEV
#define BOARD_UART_DEV     (&g_whalUart)
#define BOARD_SPI_DEV      (&g_whalSpi)
#define BOARD_FLASH_DEV    ((whal_Flash *)&whal_Stm32h5_Flash_Dev)
#define BOARD_RNG_DEV      WHAL_INTERNAL_DEV
#define BOARD_ETH_DEV      WHAL_INTERNAL_DEV
#define BOARD_ETH_PHY_DEV  WHAL_INTERNAL_DEV

/* GPIO singleton — referenced by stm32wb_gpio.c directly. */
/* GPIO dev initializer — singleton defined in driver TU. */
#define WHAL_CFG_STM32H5_GPIO_DEV { \
    .base = WHAL_STM32H563_GPIO_BASE, \
    .cfg  = (void *)&(const whal_Stm32h5_Gpio_Cfg){ \
        .pinCfg = (const whal_Stm32h5_Gpio_PinCfg[PIN_COUNT]){ \
            [LED_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_B, 0, WHAL_STM32H5_GPIO_MODE_OUT, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_LOW, \
                WHAL_STM32H5_GPIO_PULL_NONE, 0), \
            [UART_TX_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_D, 8, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_FAST, \
                WHAL_STM32H5_GPIO_PULL_UP, 7), \
            [UART_RX_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_D, 9, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_FAST, \
                WHAL_STM32H5_GPIO_PULL_UP, 7), \
            [SPI_SCK_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_A, 5, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_FAST, \
                WHAL_STM32H5_GPIO_PULL_NONE, 5), \
            [SPI_MISO_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_G, 9, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_FAST, \
                WHAL_STM32H5_GPIO_PULL_NONE, 5), \
            [SPI_MOSI_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_B, 5, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_FAST, \
                WHAL_STM32H5_GPIO_PULL_NONE, 5), \
            [SPI_CS_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_D, 14, WHAL_STM32H5_GPIO_MODE_OUT, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_FAST, \
                WHAL_STM32H5_GPIO_PULL_UP, 0), \
            [ETH_RMII_REF_CLK_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_A, 1, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_HIGH, \
                WHAL_STM32H5_GPIO_PULL_NONE, 11), \
            [ETH_RMII_MDIO_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_A, 2, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_HIGH, \
                WHAL_STM32H5_GPIO_PULL_NONE, 11), \
            [ETH_RMII_MDC_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_C, 1, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_HIGH, \
                WHAL_STM32H5_GPIO_PULL_NONE, 11), \
            [ETH_RMII_CRS_DV_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_A, 7, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_HIGH, \
                WHAL_STM32H5_GPIO_PULL_NONE, 11), \
            [ETH_RMII_RXD0_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_C, 4, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_HIGH, \
                WHAL_STM32H5_GPIO_PULL_NONE, 11), \
            [ETH_RMII_RXD1_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_C, 5, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_HIGH, \
                WHAL_STM32H5_GPIO_PULL_NONE, 11), \
            [ETH_RMII_TX_EN_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_G, 11, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_HIGH, \
                WHAL_STM32H5_GPIO_PULL_NONE, 11), \
            [ETH_RMII_TXD0_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_G, 13, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_HIGH, \
                WHAL_STM32H5_GPIO_PULL_NONE, 11), \
            [ETH_RMII_TXD1_PIN] = WHAL_STM32H5_GPIO_PIN( \
                WHAL_STM32H5_GPIO_PORT_B, 15, WHAL_STM32H5_GPIO_MODE_ALTFN, \
                WHAL_STM32H5_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32H5_GPIO_SPEED_HIGH, \
                WHAL_STM32H5_GPIO_PULL_NONE, 11), \
        }, \
        .pinCount = PIN_COUNT, \
    }, \
}

/* RNG dev initializer — singleton defined in stm32h5_rng.c. */
#define WHAL_CFG_STM32H5_RNG_DEV { \
    .base = WHAL_STM32H563_RNG_BASE, \
    .cfg  = (void *)&(const whal_Stm32h5_Rng_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

/* Flash dev initializer — singleton defined in stm32h5_flash.c. */
#define WHAL_CFG_STM32H5_FLASH_DEV { \
    .driver = WHAL_STM32H563_FLASH_DRIVER, \
    .base   = WHAL_STM32H563_FLASH_BASE, \
    .cfg    = (void *)&(const whal_Stm32h5_Flash_Cfg){ \
        .startAddr = 0x08000000, \
        .size      = 0x200000, /* 2 MB */ \
        .timeout   = &g_whalTimeout, \
    }, \
}

/* ETH descriptor rings + buffer pool — defined in wolfHAL_board.c, addresses
 * captured by the ETH singleton's cfg below at compile time. */
#define BOARD_ETH_TX_DESC_COUNT 4
#define BOARD_ETH_RX_DESC_COUNT 4
#define BOARD_ETH_TX_BUF_SIZE   1536
#define BOARD_ETH_RX_BUF_SIZE   1536

extern whal_Stm32h5_Eth_TxDesc ethTxDescs[BOARD_ETH_TX_DESC_COUNT];
extern whal_Stm32h5_Eth_RxDesc ethRxDescs[BOARD_ETH_RX_DESC_COUNT];
extern uint8_t ethTxBufs[BOARD_ETH_TX_DESC_COUNT * BOARD_ETH_TX_BUF_SIZE];
extern uint8_t ethRxBufs[BOARD_ETH_RX_DESC_COUNT * BOARD_ETH_RX_BUF_SIZE];

/* ETH dev initializer — singleton defined in stm32h5_eth.c. */
#define WHAL_CFG_STM32H5_ETH_DEV { \
    .base    = WHAL_STM32H563_ETH_BASE, \
    .macAddr = {0x00, 0x80, 0xE1, 0x00, 0x00, 0x01}, \
    .cfg     = (void *)&(const whal_Stm32h5_Eth_Cfg){ \
        .txDescs     = ethTxDescs, \
        .txBufs      = ethTxBufs, \
        .txDescCount = BOARD_ETH_TX_DESC_COUNT, \
        .txBufSize   = BOARD_ETH_TX_BUF_SIZE, \
        .rxDescs     = ethRxDescs, \
        .rxBufs      = ethRxBufs, \
        .rxDescCount = BOARD_ETH_RX_DESC_COUNT, \
        .rxBufSize   = BOARD_ETH_RX_BUF_SIZE, \
        .mdioCr      = 4, /* HCLK 168 MHz -> MDC = 168/102 ~= 1.6 MHz */ \
        .timeout     = &g_whalTimeout, \
    }, \
}

/* LAN8742A PHY dev initializer — singleton defined in lan8742a_eth_phy.c. */
#define WHAL_CFG_LAN8742A_DEV { \
    .eth  = NULL, \
    .addr = BOARD_ETH_PHY_ADDR, \
    .cfg  = (void *)&(const whal_Lan8742a_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

/* SysTick dev initializer — singleton defined in systick.c. */
#define WHAL_CFG_SYSTICK_DEV { \
    .base = WHAL_CORTEX_M33_SYSTICK_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_SysTick_Cfg){ \
        .cyclesPerTick = 168000000 / 1000, \
        .clkSrc  = WHAL_SYSTICK_CLKSRC_SYSCLK, \
        .tickInt = WHAL_SYSTICK_TICKINT_ENABLED, \
    }, \
}

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#endif /* WOLFHAL_BOARD_H */
