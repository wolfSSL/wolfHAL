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

/* Board configuration for the WeAct BlackPill STM32F411CEU6 */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"
#include <wolfHAL/platform/st/stm32f411xx.h>
#include "wolfHAL_peripheral.h"

/* SysTick timing */
volatile uint32_t g_tick = 0;

void SysTick_Handler()
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

/* STM32F411CE sector layout (512 KB). Referenced by the flash singleton's
 * cfg in wolfHAL_board.h, so this must be globally visible (not static). */
const whal_Stm32f4_Flash_Sector g_flashSectors[FLASH_SECTOR_COUNT] = {
    { .addr = 0x08000000, .size = 0x04000 },  /* Sector 0: 16 KB */
    { .addr = 0x08004000, .size = 0x04000 },  /* Sector 1: 16 KB */
    { .addr = 0x08008000, .size = 0x04000 },  /* Sector 2: 16 KB */
    { .addr = 0x0800C000, .size = 0x04000 },  /* Sector 3: 16 KB */
    { .addr = 0x08010000, .size = 0x10000 },  /* Sector 4: 64 KB */
    { .addr = 0x08020000, .size = 0x20000 },  /* Sector 5: 128 KB */
    { .addr = 0x08040000, .size = 0x20000 },  /* Sector 6: 128 KB */
    { .addr = 0x08060000, .size = 0x20000 },  /* Sector 7: 128 KB */
};

static const whal_Stm32f4_Rcc_PeriphClk g_periphClks[] = {
    {WHAL_STM32F411_GPIOA_CLOCK},
    {WHAL_STM32F411_GPIOC_CLOCK},
    {WHAL_STM32F411_USART2_CLOCK},
    {WHAL_STM32F411_SPI1_CLOCK},
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))


/* UART */
whal_Uart g_whalUart = {
    .base = WHAL_STM32F411_USART2_BASE,
    /* .driver: direct API mapping */

    .cfg = &(whal_Stm32f4_Uart_Cfg) {
        .timeout = &g_whalTimeout,
        .brr = WHAL_STM32F4_UART_BRR(50000000, 115200),
    },
};

/* SPI */
whal_Spi g_whalSpi = {
    .base = WHAL_STM32F411_SPI1_BASE,
    /* .driver: direct API mapping */

    .cfg = &(whal_Stm32f4_Spi_Cfg) {
        .pclk = 100000000,
        .timeout = &g_whalTimeout,
    },
};

void Board_WaitMs(size_t ms)
{
    uint32_t startCount = g_tick;
    while ((g_tick - startCount) < ms)
        ;
}

/*
 * Flash latency for 100 MHz at 2.7-3.6V: 3 wait states (Table 5 in RM0383)
 *
 * RCC_CFGR APB1 prescaler (PPRE1[2:0], bits 12:10):
 *   100 = AHB clock divided by 2 => APB1 = 50 MHz
 * APB2 prescaler (PPRE2[2:0], bits 15:13):
 *   0xx = AHB clock not divided => APB2 = 100 MHz
 */

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Set flash latency before increasing clock speed */
    err = whal_Stm32f4_Flash_Ext_SetLatency(BOARD_FLASH_DEV,
                                            WHAL_STM32F4_FLASH_LATENCY_3);
    if (err)
        return err;

    /* HSE 25 MHz -> PLL (25/25 * 200 / 2 = 100 MHz) -> SYSCLK = PLL */
    err = whal_Stm32f4_Rcc_EnableOsc(
        &(whal_Stm32f4_Rcc_OscCfg){WHAL_STM32F4_RCC_HSE_CFG});
    if (err)
        return err;
    err = whal_Stm32f4_Rcc_EnablePll(&(whal_Stm32f4_Rcc_PllCfg){
        .clkSrc = WHAL_STM32F4_RCC_PLLCLK_SRC_HSE,
        .m = 25, .n = 200, .p = 0, .q = 4,
    });
    if (err)
        return err;
    /* APB1 = SYSCLK/2 = 50 MHz, APB2 = SYSCLK/1 = 100 MHz */
    err = whal_Stm32f4_Rcc_SetBusPrescalers(
        &(whal_Stm32f4_Rcc_BusCfg){.ppre1 = 4, .ppre2 = 0});
    if (err)
        return err;
    err = whal_Stm32f4_Rcc_SetSysClock(WHAL_STM32F4_RCC_SYSCLK_SRC_PLL);
    if (err)
        return err;

    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32f4_Rcc_EnablePeriphClk(&g_periphClks[i]);
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

    err = whal_Uart_Deinit(&g_whalUart);
    if (err)
        return err;

    err = whal_Gpio_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    for (size_t i = PERIPH_CLK_COUNT; i-- > 0; ) {
        err = whal_Stm32f4_Rcc_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    err = whal_Stm32f4_Rcc_SetSysClock(WHAL_STM32F4_RCC_SYSCLK_SRC_HSI);
    if (err)
        return err;
    err = whal_Stm32f4_Rcc_DisablePll();
    if (err)
        return err;

    return WHAL_SUCCESS;
}
