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

/* Board configuration for the STM32U5A5ZJ Nucleo (modelled on NUCLEO-U5A5ZJ-Q). */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"
#include <wolfHAL/platform/st/stm32u5a5zj.h>
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

/* Clock: PLL1 from HSI16, targeting 160 MHz (max with EPOD booster).
 *   HSI16 = 16 MHz
 *   PLL1M = 1 -> ref_ck = 16 MHz
 *   PLL1N = 20 -> VCO = 16 * 20 = 320 MHz
 *   PLL1R = 2 -> pll1rclk = 160 MHz
 *   PLL1RGE = 3 (8-16 MHz range)
 *   PLL1MBOOST = 0 (bypass — EPOD input = PLL1 input)
 */
static const whal_Stm32u5_Rcc_PeriphClk g_flashClock = {WHAL_STM32U5A5_FLASH_CLOCK};

static const whal_Stm32u5_Rcc_PeriphClk g_periphClks[] = {
    {WHAL_STM32U5A5_GPIOA_CLOCK},
    {WHAL_STM32U5A5_GPIOB_CLOCK},
    {WHAL_STM32U5A5_GPIOC_CLOCK},
    {WHAL_STM32U5A5_USART1_CLOCK},
    {WHAL_STM32U5A5_SPI1_CLOCK},
    {WHAL_STM32U5A5_RNG_CLOCK},
    {WHAL_STM32U5A5_AES_CLOCK},
    {WHAL_STM32U5A5_HASH_CLOCK},
    {WHAL_STM32U5A5_I2C1_CLOCK},
#ifdef BOARD_WATCHDOG_WWDG
    {WHAL_STM32U5A5_WWDG_CLOCK},
#endif
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))

/* I2C */
whal_I2c g_whalI2c = {
    .base = WHAL_STM32U5A5_I2C1_BASE,
    .driver = WHAL_STM32U5A5_I2C1_DRIVER,

    .cfg = &(whal_Stm32u5_I2c_Cfg) {
        .pclk = 160000000,
        .timeout = &g_whalTimeout,
    },
};

/* SPI */
whal_Spi g_whalSpi = {
    .base = WHAL_STM32U5A5_SPI1_BASE,
    .driver = WHAL_STM32U5A5_SPI1_DRIVER,

    .cfg = &(whal_Stm32u5_Spi_Cfg) {
        .pclk = 160000000,
        .timeout = &g_whalTimeout,
    },
};

/* DMA */
#ifdef BOARD_DMA

whal_Dma g_whalDma1 = {
    .base = WHAL_STM32U5A5_GPDMA1_BASE,
    .driver = WHAL_STM32U5A5_GPDMA1_DRIVER,
    .cfg = &(whal_Stm32u5_Gpdma_Cfg){
        .numChannels = 8,
        .timeout = &g_whalTimeout,
    },
};

static const whal_Stm32u5_Rcc_PeriphClk g_dmaClock = {WHAL_STM32U5A5_GPDMA1_CLOCK};

void GPDMA1_Channel0_IRQHandler(void)
{
    whal_Stm32u5_Gpdma_IRQHandler(&g_whalDma1, 0, NULL, NULL);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    whal_Stm32u5_Gpdma_IRQHandler(&g_whalDma1, 1, NULL, NULL);
}
#endif

/* UART (USART1 via VCP at 115200 baud) */
whal_Uart g_whalUart = {
    .base = WHAL_STM32U5A5_USART1_BASE,
    .driver = WHAL_STM32U5A5_USART1_DRIVER,

    .cfg = &(whal_Stm32u5_Uart_Cfg) {
        .timeout = &g_whalTimeout,
        .brr = WHAL_STM32U5_UART_BRR(160000000, 115200),
    },
};

/* RNG, AES + mode, HASH + algorithm singletons are defined in their driver TUs
 * from WHAL_CFG_* initializers in wolfHAL_board.h. */

/* Hash (HASH hardware accelerator) — vtable dispatcher for whal_Crypto_Init/Deinit. */
whal_Crypto g_whalHash = {
    .base = WHAL_STM32U5A5_HASH_BASE,
    .driver = &whal_Stm32u5_Hash_CryptoDriver,

    .cfg = &(whal_Stm32u5_Hash_Cfg) {
        .timeout = &g_whalTimeout,
    },
};

#ifdef BOARD_WATCHDOG_IWDG
whal_Watchdog g_whalWatchdog = {
    .base = WHAL_STM32U5A5_IWDG_BASE,
    .driver = WHAL_STM32U5A5_IWDG_DRIVER,

    .cfg = &(whal_Stm32u5_Iwdg_Cfg) {
        .prescaler = WHAL_STM32U5_IWDG_PR_32,
        .reload = 100,
        .timeout = &g_whalTimeout,
    },
};
#elif defined(BOARD_WATCHDOG_WWDG)
whal_Watchdog g_whalWatchdog = {
    .base = WHAL_STM32U5A5_WWDG_BASE,
    .driver = WHAL_STM32U5A5_WWDG_DRIVER,

    .cfg = &(whal_Stm32u5_Wwdg_Cfg) {
        .prescaler = WHAL_STM32U5_WWDG_TB_128,
        .window = 0x7F,
        .counter = 0x7F,
    },
};
#endif

void Board_WaitMs(size_t ms)
{
    uint32_t startCount = g_tick;
    while (g_tick - startCount < ms);
}

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Enable PWR clock (RCC_AHB3ENR bit 2) — required to access PWR registers. */
    static const whal_Stm32u5_Rcc_PeriphClk pwrClock = {WHAL_STM32U5A5_PWR_CLOCK};
    err = whal_Stm32u5_Rcc_EnablePeriphClk(&pwrClock);
    if (err)
        return err;

    /* Switch to voltage scaling Range 1 BEFORE raising SYSCLK above 24 MHz. */
    err = whal_Stm32u5_Pwr_SetVosRange(WHAL_STM32U5_PWR_VOS_RANGE_1);
    if (err)
        return err;

    /* HSI16 -> PLL1 (M=1, N=20, R=2 -> 160 MHz) -> SYSCLK = PLL1 */
    err = whal_Stm32u5_Rcc_EnableOsc(
        &(whal_Stm32u5_Rcc_OscCfg){WHAL_STM32U5_RCC_HSI16_CFG});
    if (err)
        return err;
    err = whal_Stm32u5_Rcc_EnablePll1(&(whal_Stm32u5_Rcc_Pll1Cfg){
        .clkSrc = WHAL_STM32U5_RCC_PLL1SRC_HSI16,
        .rge    = WHAL_STM32U5_RCC_PLL1RGE_8_16,
        .m      = 1,  /* /1 -> ref = 16 MHz */
        .mboost = 0,  /* bypass EPOD prescaler */
        .n      = 20, /* x20 -> VCO = 320 MHz */
        .r      = 2,  /* /2 -> SYSCLK = 160 MHz */
        .q      = 2,
        .p      = 2,
    });
    if (err)
        return err;

    /* EPOD booster is required for SYSCLK above 55 MHz in Range 1. */
    err = whal_Stm32u5_Pwr_EnableEpodBooster();
    if (err)
        return err;

    /* Enable flash clock and set latency before increasing clock speed.
     * At 160 MHz, 3.3 V: 4 wait states required (RM0456 Table 36). */
    err = whal_Stm32u5_Rcc_EnablePeriphClk(&g_flashClock);
    if (err)
        return err;

    err = whal_Stm32u5_Flash_Ext_SetLatency(BOARD_FLASH_DEV, 4);
    if (err)
        return err;

    err = whal_Stm32u5_Rcc_SetSysClock(WHAL_STM32U5_RCC_SYSCLK_SRC_PLL1);
    if (err)
        return err;

#ifdef BOARD_WATCHDOG_IWDG
    /* Enable LSI oscillator required by IWDG */
    err = whal_Stm32u5_Rcc_EnableLsi();
    if (err)
        return err;
#endif

    /* Select HSI16 as RNG kernel clock source. */
    err = whal_Stm32u5_Rcc_SetRngClockSrc(WHAL_STM32U5_RCC_RNGSEL_HSI16);
    if (err)
        return err;

    /* Enable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32u5_Rcc_EnablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    err = whal_Irq_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

#ifdef BOARD_DMA
    err = whal_Stm32u5_Rcc_EnablePeriphClk(&g_dmaClock);
    if (err)
        return err;
    err = whal_Dma_Init(&g_whalDma1);
    if (err)
        return err;

    /* Enable NVIC interrupts for GPDMA1 channel 0 (IRQ 27) and channel 1 (IRQ 28). */
    err = whal_Irq_Enable(WHAL_INTERNAL_DEV, 27, NULL);
    if (err)
        return err;
    err = whal_Irq_Enable(WHAL_INTERNAL_DEV, 28, NULL);
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

    err = whal_Flash_Init(BOARD_FLASH_DEV);
    if (err)
        return err;

    err = whal_Rng_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Crypto_Init(&g_whalHash);
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

    err = whal_Crypto_Deinit(&g_whalHash);
    if (err)
        return err;

    err = whal_Rng_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Flash_Deinit(BOARD_FLASH_DEV);
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
    whal_Irq_Disable(WHAL_INTERNAL_DEV, 27);
    whal_Irq_Disable(WHAL_INTERNAL_DEV, 28);

    err = whal_Dma_Deinit(&g_whalDma1);
    if (err)
        return err;
    err = whal_Stm32u5_Rcc_DisablePeriphClk(&g_dmaClock);
    if (err)
        return err;
#endif

    err = whal_Irq_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    /* Disable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32u5_Rcc_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

#ifdef BOARD_WATCHDOG_IWDG
    err = whal_Stm32u5_Rcc_DisableLsi();
    if (err)
        return err;
#endif

    err = whal_Stm32u5_Rcc_SetSysClock(WHAL_STM32U5_RCC_SYSCLK_SRC_HSI16);
    if (err)
        return err;
    err = whal_Stm32u5_Rcc_DisablePll1();
    if (err)
        return err;

    /* Reduce flash latency then disable flash clock */
    err = whal_Stm32u5_Flash_Ext_SetLatency(BOARD_FLASH_DEV, 0);
    if (err)
        return err;

    err = whal_Stm32u5_Rcc_DisablePeriphClk(&g_flashClock);
    if (err)
        return err;

    return WHAL_SUCCESS;
}
