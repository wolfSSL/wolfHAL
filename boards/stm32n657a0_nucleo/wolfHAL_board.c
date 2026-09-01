/* wolfHAL_board.c
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

/* Board configuration for the STM32N657A0 Nucleo-144 (NUCLEO-N657X0-Q) */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"
#include <wolfHAL/platform/st/stm32n657a0.h>
#include <wolfHAL/eth_phy/lan8742a_eth_phy.h>
#include "wolfHAL_peripheral.h"

/* SysTick timing */
volatile uint32_t g_tick = 0;

void SysTick_Handler(void)
{
    g_tick++;
}

uint32_t Board_GetTick(void)
{
    return g_tick;
}

whal_Timeout g_whalTimeout = {
    .timeoutTicks = 1000, /* 1s timeout */
    .GetTick = Board_GetTick,
};

/* Clock: HSI at 64 MHz (default after reset).
 * The STM32N6 boots from ROM into HSI; PLL setup is complex (PLL1+IC dividers).
 * For initial bring-up, run at HSI 64 MHz. */
/* API is directly mapped */
static const whal_Stm32n6_Rcc_PeriphClk g_periphClks[] = {
    {WHAL_STM32N657_GPIOA_CLOCK},
    {WHAL_STM32N657_GPIOB_CLOCK},
    {WHAL_STM32N657_GPIOE_CLOCK},
    {WHAL_STM32N657_GPIOF_CLOCK},
    {WHAL_STM32N657_GPIOG_CLOCK},
    {WHAL_STM32N657_USART1_CLOCK},
    {WHAL_STM32N657_SPI1_CLOCK},
    {WHAL_STM32N657_I2C1_CLOCK},
    {WHAL_STM32N657_RNG_CLOCK},
    {WHAL_STM32N657_CRYP_CLOCK},
    {WHAL_STM32N657_HASH_CLOCK},
#ifdef BOARD_WATCHDOG_WWDG
    {WHAL_STM32N657_WWDG_CLOCK},
#endif
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))

static const whal_Stm32n6_Rcc_PeriphClk g_ethClocks[] = {
    {WHAL_STM32N657_ETH1MAC_CLOCK},
    {WHAL_STM32N657_ETH1TX_CLOCK},
    {WHAL_STM32N657_ETH1RX_CLOCK},
    {WHAL_STM32N657_ETH1_CLOCK},
};
#define ETH_PERIPH_CLK_COUNT (sizeof(g_ethClocks) / sizeof(g_ethClocks[0]))

/* GPIO */
/* API is directly mapped */

/* I2C */
/* API is directly mapped */
whal_I2c g_whalI2c = {
    .base = WHAL_STM32N657_I2C1_BASE,

    .cfg = &(whal_Stm32n6_I2c_Cfg) {
        .pclk = 64000000,
        .timeout = &g_whalTimeout,
    },
};

/* SPI */
/* API is directly mapped */
whal_Spi g_whalSpi = {
    .base = WHAL_STM32N657_SPI1_BASE,

    .cfg = &(whal_Stm32n6_Spi_Cfg) {
        .pclk = 64000000,
        .timeout = &g_whalTimeout,
    },
};


/* DMA */
#ifdef BOARD_DMA
/* API is directly mapped */
whal_Dma g_whalDma1 = {
    .base = WHAL_STM32N657_GPDMA1_BASE,
    .cfg = &(whal_Stm32n6_Gpdma_Cfg){
        .numChannels = 16,
        .timeout = &g_whalTimeout,
    },
};

static const whal_Stm32n6_Rcc_PeriphClk g_dmaClock = {WHAL_STM32N657_GPDMA1_CLOCK};
#endif

/* UART (USART1 via VCP at 115200 baud, 64 MHz HSI) */
/* API is directly mapped */
whal_Uart g_whalUart = {
    .base = WHAL_STM32N657_USART1_BASE,

    .cfg = &(whal_Stm32n6_Uart_Cfg) {
        .timeout = &g_whalTimeout,
        .brr = WHAL_STM32N6_UART_BRR(32000000, 115200),
    },
};

/* RNG, ETH, EthPhy, AES mode, HASH algorithm singletons live in wolfHAL_board.h as
 * `static const`. */

/* Crypto (CRYP hardware accelerator) — vtable dispatcher for whal_Crypto_Init/Deinit. */
whal_Crypto g_whalCrypto = {
    .base = WHAL_STM32N657_CRYP_BASE,
    .driver = &whal_Stm32n6_Cryp_CryptoDriver,

    .cfg = &(whal_Stm32n6_Cryp_Cfg) {
        .timeout = &g_whalTimeout,
    },
};

/* Hash (HASH hardware accelerator) — vtable dispatcher for whal_Crypto_Init/Deinit. */
whal_Crypto g_whalHash = {
    .base = WHAL_STM32N657_HASH_BASE,
    .driver = &whal_Stm32n6_Hash_CryptoDriver,

    .cfg = &(whal_Stm32n6_Hash_Cfg) {
        .timeout = &g_whalTimeout,
    },
};

#ifdef BOARD_WATCHDOG_IWDG
whal_Watchdog g_whalWatchdog = {
    .base = WHAL_STM32N657_IWDG_BASE,
    .driver = WHAL_STM32N657_IWDG_DRIVER,

    .cfg = &(whal_Stm32n6_Iwdg_Cfg) {
        .prescaler = WHAL_STM32N6_IWDG_PR_32,
        .reload = 100,
        .timeout = &g_whalTimeout,
    },
};
#elif defined(BOARD_WATCHDOG_WWDG)
whal_Watchdog g_whalWatchdog = {
    .base = WHAL_STM32N657_WWDG_BASE,
    .driver = WHAL_STM32N657_WWDG_DRIVER,

    .cfg = &(whal_Stm32n6_Wwdg_Cfg) {
        .prescaler = WHAL_STM32N6_WWDG_TB_128,
        .window = 0x7F,
        .counter = 0x7F,
    },
};
#endif

/* Ethernet */
/* ETH DMA descriptors and frame buffers must live in AXI-master-visible
 * RAM. The default RAM region (FLEXRAM at 0x34000000) is allocated as
 * Cortex-M55 TCM and is not reachable by the ETH AXI master, so place
 * these in AXISRAM1 via the .axisram1 section. */
whal_Stm32n6_Eth_TxDesc ethTxDescs[BOARD_ETH_TX_DESC_COUNT]
    __attribute__((aligned(16), section(".axisram1")));
whal_Stm32n6_Eth_RxDesc ethRxDescs[BOARD_ETH_RX_DESC_COUNT]
    __attribute__((aligned(16), section(".axisram1")));
uint8_t ethTxBufs[BOARD_ETH_TX_DESC_COUNT * BOARD_ETH_TX_BUF_SIZE]
    __attribute__((aligned(8), section(".axisram1")));
uint8_t ethRxBufs[BOARD_ETH_RX_DESC_COUNT * BOARD_ETH_RX_BUF_SIZE]
    __attribute__((aligned(8), section(".axisram1")));

void Board_WaitMs(size_t ms)
{
    uint32_t startCount = g_tick;
    while ((g_tick - startCount) < ms)
        ;
}

/* Diagnostic: grant the ETH1 AXI master CID=1 / secure / privileged so
 * RISAF2 (default region) lets it read/write AXISRAM1 descriptors and
 * frame buffers. Refactor into a proper RIF helper if this fixes the
 * loopback test. */
static void Board_AllowEth1Master(void)
{
    /* RIFSC secure alias 0x54024000; ETH1 = RIMU index 6, RISUP index 60. */

    /* Mark ETH1 (slave index 60) as secure + privileged. Without this,
     * the "secure guard" forces RIMC_ATTR6.MSEC to 0 because the ETH1
     * config port is nonsecure-accessible by default. */
    *(volatile uint32_t *)(0x54024000U + 0x014U) |= (1U << 28); /* SECCFGR1 */
    *(volatile uint32_t *)(0x54024000U + 0x034U) |= (1U << 28); /* PRIVCFGR1 */

    /* RIMC_ATTR6: MPRIV=1, MSEC=1, MCID=1 — ETH1 master tags AXI
     * transactions to match RISAF2 default-region policy. */
    *(volatile uint32_t *)(0x54024000U + 0xC10U + 6U * 4U) =
        (1U << 9) | (1U << 8) | (1U << 4);
}

whal_Error Board_Init(void)
{
    whal_Error err;

    Board_AllowEth1Master();

    /* HSI 64 MHz: enable, then select as both system and CPU clocks. */
    err = whal_Stm32n6_Rcc_EnableOsc(
        &(whal_Stm32n6_Rcc_OscCfg){WHAL_STM32N6_RCC_HSI_CFG});
    if (err)
        return err;
    err = whal_Stm32n6_Rcc_SetCpuClock(WHAL_STM32N6_RCC_CPUCLK_SRC_HSI);
    if (err)
        return err;
    err = whal_Stm32n6_Rcc_SetSysClock(WHAL_STM32N6_RCC_SYSCLK_SRC_HSI);
    if (err)
        return err;

    /* Enable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32n6_Rcc_EnablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    /* Select RMII for the ETH1 MAC-PHY interface. Must precede the ETH1
     * clock-enable loop per RM0486 §14.10.51. */
    err = whal_Stm32n6_Rcc_SetEth1If(WHAL_STM32N6_RCC_ETH1_IF_RMII);
    if (err)
        return err;

    /* Enable ETH clocks */
    for (size_t i = 0; i < ETH_PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32n6_Rcc_EnablePeriphClk(&g_ethClocks[i]);
        if (err)
            return err;
    }

    err = whal_Irq_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

#ifdef BOARD_DMA
    err = whal_Stm32n6_Rcc_EnablePeriphClk(&g_dmaClock);
    if (err)
        return err;
    err = whal_Dma_Init(&g_whalDma1);
    if (err)
        return err;
#endif

    err = whal_Gpio_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Uart_Init(&g_whalUart);
    if (err)
        return err;

    err = whal_Spi_Init(&g_whalSpi);
    if (err)
        return err;

    err = whal_I2c_Init(&g_whalI2c);
    if (err)
        return err;

    err = whal_Rng_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Crypto_Init(&g_whalCrypto);
    if (err)
        return err;

    err = whal_Crypto_Init(&g_whalHash);
    if (err)
        return err;

    err = whal_Eth_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_EthPhy_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Timer_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Timer_Start(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = Peripheral_Init();
    if (err)
        return err;

    return WHAL_SUCCESS;
}

whal_Error Board_Deinit(void)
{
    whal_Error err;

    err = Peripheral_Deinit();
    if (err)
        return err;

    err = whal_Timer_Stop(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Timer_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_EthPhy_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Eth_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Crypto_Deinit(&g_whalHash);
    if (err)
        return err;

    err = whal_Crypto_Deinit(&g_whalCrypto);
    if (err)
        return err;

    err = whal_Rng_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_I2c_Deinit(&g_whalI2c);
    if (err)
        return err;

    err = whal_Spi_Deinit(&g_whalSpi);
    if (err)
        return err;

    err = whal_Uart_Deinit(&g_whalUart);
    if (err)
        return err;

    err = whal_Gpio_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

#ifdef BOARD_DMA
    err = whal_Dma_Deinit(&g_whalDma1);
    if (err)
        return err;
    err = whal_Stm32n6_Rcc_DisablePeriphClk(&g_dmaClock);
    if (err)
        return err;
#endif

    err = whal_Irq_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    /* Disable ETH clocks */
    for (size_t i = 0; i < ETH_PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32n6_Rcc_DisablePeriphClk(&g_ethClocks[i]);
        if (err)
            return err;
    }

    /* Disable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32n6_Rcc_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    /* HSI is the post-Deinit fallback; nothing else to tear down. */

    return WHAL_SUCCESS;
}
