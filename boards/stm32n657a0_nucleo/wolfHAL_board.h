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
#include <wolfHAL/platform/st/stm32n657a0.h>
#include <wolfHAL/eth_phy/lan8742a_eth_phy.h>
#include <wolfHAL/crypto/stm32n6_cryp.h>
#include <wolfHAL/crypto/stm32n6_hash.h>
#include <wolfHAL/rng/stm32n6_rng.h>

extern whal_Uart g_whalUart;
extern whal_Spi g_whalSpi;
extern whal_I2c g_whalI2c;
extern whal_Crypto g_whalCrypto;
extern whal_Crypto g_whalHash;
extern whal_Watchdog g_whalWatchdog;
#ifdef BOARD_DMA
extern whal_Dma g_whalDma1;
#endif

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

/* Ethernet PHY: LAN8742A on MDIO address 0 */
#define BOARD_ETH_PHY_ADDR 0
#define BOARD_ETH_PHY_ID1  0x0007
#define BOARD_ETH_PHY_ID2  0xC131

/* BOARD_*_DEV: how this board reaches each peripheral. */
#define BOARD_GPIO_DEV         WHAL_INTERNAL_DEV
#define BOARD_UART_DEV         (&g_whalUart)
#define BOARD_SPI_DEV          (&g_whalSpi)
#define BOARD_I2C_DEV          (&g_whalI2c)
#define BOARD_WATCHDOG_DEV     (&g_whalWatchdog)
#define BOARD_RNG_DEV          WHAL_INTERNAL_DEV
#define BOARD_ETH_DEV          WHAL_INTERNAL_DEV
#define BOARD_ETH_PHY_DEV      WHAL_INTERNAL_DEV
#define BOARD_AES_ECB_DEV      WHAL_INTERNAL_DEV
#define BOARD_AES_CBC_DEV      WHAL_INTERNAL_DEV
#define BOARD_AES_CTR_DEV      WHAL_INTERNAL_DEV
#define BOARD_AES_GCM_DEV      WHAL_INTERNAL_DEV
#define BOARD_AES_GMAC_DEV     WHAL_INTERNAL_DEV
#define BOARD_AES_CCM_DEV      WHAL_INTERNAL_DEV
#define BOARD_SHA1_DEV         WHAL_INTERNAL_DEV
#define BOARD_SHA224_DEV       WHAL_INTERNAL_DEV
#define BOARD_SHA256_DEV       WHAL_INTERNAL_DEV
#define BOARD_HMAC_SHA1_DEV    WHAL_INTERNAL_DEV
#define BOARD_HMAC_SHA224_DEV  WHAL_INTERNAL_DEV
#define BOARD_HMAC_SHA256_DEV  WHAL_INTERNAL_DEV

/* IWDG/WWDG dev initializers — singletons defined in stm32wb_iwdg.c /
 * stm32wb_wwdg.c (stm32n6 is an include alias). */
#define WHAL_CFG_STM32N6_IWDG_DEV { \
    .base = WHAL_STM32N657_IWDG_BASE, \
    .cfg  = (void *)&(const whal_Stm32n6_Iwdg_Cfg){ \
        .prescaler = WHAL_STM32N6_IWDG_PR_32, \
        .reload    = 100, \
        .timeout   = &g_whalTimeout, \
    }, \
}

#define WHAL_CFG_STM32N6_WWDG_DEV { \
    .base = WHAL_STM32N657_WWDG_BASE, \
    .cfg  = (void *)&(const whal_Stm32n6_Wwdg_Cfg){ \
        .prescaler = WHAL_STM32N6_WWDG_TB_128, \
        .window    = 0x7F, \
        .counter   = 0x7F, \
    }, \
}

/* GPIO dev initializer — singleton defined in driver TU. */
#define WHAL_CFG_STM32N6_GPIO_DEV { \
    .base = WHAL_STM32N657_GPIO_BASE, \
    .cfg = (void *)&(const whal_Stm32n6_Gpio_Cfg){ \
        .pinCfg = (const whal_Stm32n6_Gpio_PinCfg[PIN_COUNT]){ \
            /* LD1 Green LED on PB0 */ \
            [LED_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_B, 0, WHAL_STM32N6_GPIO_MODE_OUT, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_LOW, \
                WHAL_STM32N6_GPIO_PULL_NONE, 0), \
            /* USART1 TX on PE5, AF7 */ \
            [UART_TX_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_E, 5, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_FAST, \
                WHAL_STM32N6_GPIO_PULL_UP, 7), \
            /* USART1 RX on PE6, AF7 */ \
            [UART_RX_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_E, 6, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_FAST, \
                WHAL_STM32N6_GPIO_PULL_UP, 7), \
            /* SPI1 SCK on PA5, AF5 */ \
            [SPI_SCK_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_A, 5, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_FAST, \
                WHAL_STM32N6_GPIO_PULL_NONE, 5), \
            /* SPI1 MISO on PA6, AF5 */ \
            [SPI_MISO_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_A, 6, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_FAST, \
                WHAL_STM32N6_GPIO_PULL_NONE, 5), \
            /* SPI1 MOSI on PA7, AF5 */ \
            [SPI_MOSI_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_A, 7, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_FAST, \
                WHAL_STM32N6_GPIO_PULL_NONE, 5), \
            /* SPI CS on PA4, output */ \
            [SPI_CS_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_A, 4, WHAL_STM32N6_GPIO_MODE_OUT, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_FAST, \
                WHAL_STM32N6_GPIO_PULL_UP, 0), \
            /* I2C1 SCL on PB6, AF4, open-drain */ \
            [I2C_SCL_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_B, 6, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_OPENDRAIN, WHAL_STM32N6_GPIO_SPEED_FAST, \
                WHAL_STM32N6_GPIO_PULL_UP, 4), \
            /* I2C1 SDA on PB7, AF4, open-drain */ \
            [I2C_SDA_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_B, 7, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_OPENDRAIN, WHAL_STM32N6_GPIO_SPEED_FAST, \
                WHAL_STM32N6_GPIO_PULL_UP, 4), \
            /* RMII REF_CLK on PF7, AF11 */ \
            [ETH_RMII_REF_CLK_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_F, 7, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_HIGH, \
                WHAL_STM32N6_GPIO_PULL_NONE, 11), \
            /* RMII MDIO on PF4, AF11 */ \
            [ETH_RMII_MDIO_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_F, 4, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_HIGH, \
                WHAL_STM32N6_GPIO_PULL_NONE, 11), \
            /* RMII MDC on PG11, AF11 */ \
            [ETH_RMII_MDC_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_G, 11, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_HIGH, \
                WHAL_STM32N6_GPIO_PULL_NONE, 11), \
            /* RMII CRS_DV on PF10, AF11 */ \
            [ETH_RMII_CRS_DV_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_F, 10, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_HIGH, \
                WHAL_STM32N6_GPIO_PULL_NONE, 11), \
            /* RMII RXD0 on PF14, AF11 */ \
            [ETH_RMII_RXD0_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_F, 14, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_HIGH, \
                WHAL_STM32N6_GPIO_PULL_NONE, 11), \
            /* RMII RXD1 on PF15, AF11 */ \
            [ETH_RMII_RXD1_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_F, 15, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_HIGH, \
                WHAL_STM32N6_GPIO_PULL_NONE, 11), \
            /* RMII TX_EN on PF11, AF11 */ \
            [ETH_RMII_TX_EN_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_F, 11, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_HIGH, \
                WHAL_STM32N6_GPIO_PULL_NONE, 11), \
            /* RMII TXD0 on PF12, AF11 */ \
            [ETH_RMII_TXD0_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_F, 12, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_HIGH, \
                WHAL_STM32N6_GPIO_PULL_NONE, 11), \
            /* RMII TXD1 on PF13, AF11 */ \
            [ETH_RMII_TXD1_PIN] = WHAL_STM32N6_GPIO_PIN( \
                WHAL_STM32N6_GPIO_PORT_F, 13, WHAL_STM32N6_GPIO_MODE_ALTFN, \
                WHAL_STM32N6_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32N6_GPIO_SPEED_HIGH, \
                WHAL_STM32N6_GPIO_PULL_NONE, 11), \
        }, \
        .pinCount = PIN_COUNT, \
    }, \
}

/* RNG dev initializer — singleton defined in stm32wba_rng.c (stm32n6 is
 * an include alias). */
#define WHAL_CFG_STM32N6_RNG_DEV { \
    .base = WHAL_STM32N657_RNG_BASE, \
    .cfg  = (void *)&(const whal_Stm32n6_Rng_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

/* CRYP + mode dev initializers — singletons defined in stm32n6_cryp.c. */
#define WHAL_CFG_STM32N6_CRYP_DEV { \
    .base = WHAL_STM32N657_CRYP_BASE, \
    .cfg  = (void *)&(const whal_Stm32n6_Cryp_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

#define WHAL_CFG_STM32N6_CRYP_ECB_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32n6_Cryp_Dev, \
}

#define WHAL_CFG_STM32N6_CRYP_CBC_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32n6_Cryp_Dev, \
}

#define WHAL_CFG_STM32N6_CRYP_CTR_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32n6_Cryp_Dev, \
}

#define WHAL_CFG_STM32N6_CRYP_GCM_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32n6_Cryp_Dev, \
}

#define WHAL_CFG_STM32N6_CRYP_GMAC_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32n6_Cryp_Dev, \
}

#define WHAL_CFG_STM32N6_CRYP_CCM_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32n6_Cryp_Dev, \
}

/* HASH + algorithm dev initializers — singletons defined in stm32wba_hash.c
 * (stm32n6 is an include alias). */
#define WHAL_CFG_STM32N6_HASH_DEV { \
    .base = WHAL_STM32N657_HASH_BASE, \
    .cfg  = (void *)&(const whal_Stm32n6_Hash_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

#define WHAL_CFG_STM32N6_HASH_SHA1_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wba_Hash_Dev, \
}

#define WHAL_CFG_STM32N6_HASH_SHA224_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wba_Hash_Dev, \
}

#define WHAL_CFG_STM32N6_HASH_SHA256_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wba_Hash_Dev, \
}

#define WHAL_CFG_STM32N6_HASH_HMAC_SHA1_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wba_Hash_Dev, \
}

#define WHAL_CFG_STM32N6_HASH_HMAC_SHA224_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wba_Hash_Dev, \
}

#define WHAL_CFG_STM32N6_HASH_HMAC_SHA256_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wba_Hash_Dev, \
}

/* ETH descriptor rings + buffer pool. Live in wolfHAL_board.c with the .axisram1
 * section attribute; declared here so the ETH singleton cfg below can
 * take their addresses at compile time. */
#define BOARD_ETH_TX_DESC_COUNT 4
#define BOARD_ETH_RX_DESC_COUNT 4
#define BOARD_ETH_TX_BUF_SIZE   1536
#define BOARD_ETH_RX_BUF_SIZE   1536

extern whal_Stm32n6_Eth_TxDesc ethTxDescs[BOARD_ETH_TX_DESC_COUNT];
extern whal_Stm32n6_Eth_RxDesc ethRxDescs[BOARD_ETH_RX_DESC_COUNT];
extern uint8_t ethTxBufs[BOARD_ETH_TX_DESC_COUNT * BOARD_ETH_TX_BUF_SIZE];
extern uint8_t ethRxBufs[BOARD_ETH_RX_DESC_COUNT * BOARD_ETH_RX_BUF_SIZE];

/* ETH dev initializer — singleton defined in stm32n6_eth.c. */
#define WHAL_CFG_STM32N6_ETH_DEV { \
    .base    = WHAL_STM32N657_ETH_BASE, \
    .macAddr = {0x00, 0x80, 0xE1, 0x00, 0x00, 0x01}, \
    .cfg     = (void *)&(const whal_Stm32n6_Eth_Cfg){ \
        .txDescs     = ethTxDescs, \
        .txBufs      = ethTxBufs, \
        .txDescCount = BOARD_ETH_TX_DESC_COUNT, \
        .txBufSize   = BOARD_ETH_TX_BUF_SIZE, \
        .rxDescs     = ethRxDescs, \
        .rxBufs      = ethRxBufs, \
        .rxDescCount = BOARD_ETH_RX_DESC_COUNT, \
        .rxBufSize   = BOARD_ETH_RX_BUF_SIZE, \
        .mdioCr      = 0, /* HCLK 64 MHz -> MDC = 64/42 ~= 1.5 MHz */ \
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

/* NVIC dev initializer — singleton defined in cortex_m4_nvic.c. */
#define WHAL_CFG_NVIC_DEV { \
    .base = WHAL_CORTEX_M55_NVIC_BASE, \
    /* .driver: direct API mapping */ \
}

/* SysTick dev initializer — singleton defined in systick.c. */
#define WHAL_CFG_SYSTICK_DEV { \
    .base = WHAL_CORTEX_M55_SYSTICK_BASE, \
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
