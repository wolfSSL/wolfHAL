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

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"
#include <wolfHAL/platform/st/stm32f302r8.h>
#include "wolfHAL_peripheral.h"

volatile uint32_t g_tick = 0;
volatile uint8_t g_waiting = 0;
volatile uint8_t g_tickOverflow = 0;

void SysTick_Handler()
{
    uint32_t tickBefore = g_tick++;
    if (g_waiting) {
        if (tickBefore > g_tick)
            g_tickOverflow = 1;
    }
}

uint32_t Board_GetTick(void)
{
    return g_tick;
}

whal_Timeout g_whalTimeout = {
    .timeoutTicks = 1000,
    .GetTick = Board_GetTick,
};

/* Clock — PLL at 48 MHz (HSI/2 * 12) */
static const whal_Stm32f3_Rcc_PeriphClk g_periphClks[] = {
    {WHAL_STM32F302_GPIOA_CLOCK},
    {WHAL_STM32F302_GPIOB_CLOCK},
    {WHAL_STM32F302_GPIOC_CLOCK},
    {WHAL_STM32F302_USART2_CLOCK},
    {WHAL_STM32F302_SPI3_CLOCK},
    {WHAL_STM32F302_I2C1_CLOCK},
#ifdef BOARD_WATCHDOG_WWDG
    {WHAL_STM32F302_WWDG_CLOCK},
#endif
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))


/* UART — USART2 at 115200 baud */
whal_Uart g_whalUart = {
    .base = WHAL_STM32F302_USART2_BASE,

    .cfg = &(whal_Stm32f3_Uart_Cfg) {
        .timeout = &g_whalTimeout,
        .brr = WHAL_STM32F3_UART_BRR(48000000, 115200),
    },
};

/* SPI — SPI3 */
whal_Spi g_whalSpi = {
    .base = WHAL_STM32F302_SPI3_BASE,

    .cfg = &(whal_Stm32f3_Spi_Cfg) {
        .pclk = 48000000,
        .timeout = &g_whalTimeout,
    },
};

/* I2C — I2C1 (HSI at 8 MHz per RCC_CFGR3.I2C1SW reset default) */
whal_I2c g_whalI2c = {
    .base = WHAL_STM32F302_I2C1_BASE,

    .cfg = &(whal_Stm32f3_I2c_Cfg) {
        .pclk = 8000000,
        .timeout = &g_whalTimeout,
    },
};

#ifdef BOARD_WATCHDOG_IWDG
whal_Watchdog g_whalWatchdog = {
    .base = WHAL_STM32F302_IWDG_BASE,
    .driver = WHAL_STM32F302_IWDG_DRIVER,

    .cfg = &(whal_Stm32f3_Iwdg_Cfg) {
        .prescaler = WHAL_STM32F3_IWDG_PR_64,
        .reload = 500,
        .timeout = &g_whalTimeout,
    },
};
#elif defined(BOARD_WATCHDOG_WWDG)
whal_Watchdog g_whalWatchdog = {
    .base = WHAL_STM32F302_WWDG_BASE,
    .driver = WHAL_STM32F302_WWDG_DRIVER,

    .cfg = &(whal_Stm32f3_Wwdg_Cfg) {
        .prescaler = 3,
        .window = 0x7F,
        .counter = 0x7F,
    },
};
#endif

void Board_WaitMs(size_t ms)
{
    uint32_t startCount = g_tick;
    while ((g_tick - startCount) < ms)
        ;
}

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Set flash latency before increasing clock speed.
     * STM32F3: 0 WS for HCLK <= 24 MHz, 1 WS for 24 < HCLK <= 48 MHz,
     * 2 WS for 48 < HCLK <= 72 MHz. */
    err = whal_Stm32f3_Flash_Ext_SetLatency(BOARD_FLASH_DEV,
                                             WHAL_STM32F3_FLASH_LATENCY_1);
    if (err)
        return err;

    /* HSI -> PLL (HSI/2 * 12 = 48 MHz) -> SYSCLK = PLL */
    err = whal_Stm32f3_Rcc_EnableOsc(
        &(whal_Stm32f3_Rcc_OscCfg){WHAL_STM32F3_RCC_HSI_CFG});
    if (err)
        return err;
    err = whal_Stm32f3_Rcc_EnablePll(&(whal_Stm32f3_Rcc_PllCfg){
        .clkSrc = WHAL_STM32F3_RCC_PLLSRC_HSI_DIV2,
        .prediv = 1,
        .pllmul = 12,
    });
    if (err)
        return err;
    err = whal_Stm32f3_Rcc_SetSysClock(WHAL_STM32F3_RCC_SYSCLK_SRC_PLL);
    if (err)
        return err;

    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32f3_Rcc_EnablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

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

    err = whal_Spi_Deinit(&g_whalSpi);
    if (err)
        return err;

    err = whal_I2c_Deinit(&g_whalI2c);
    if (err)
        return err;

    err = whal_Uart_Deinit(&g_whalUart);
    if (err)
        return err;

    err = whal_Gpio_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    for (size_t i = PERIPH_CLK_COUNT; i-- > 0; ) {
        err = whal_Stm32f3_Rcc_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    err = whal_Stm32f3_Rcc_SetSysClock(WHAL_STM32F3_RCC_SYSCLK_SRC_HSI);
    if (err)
        return err;
    err = whal_Stm32f3_Rcc_DisablePll();
    if (err)
        return err;

    return WHAL_SUCCESS;
}
