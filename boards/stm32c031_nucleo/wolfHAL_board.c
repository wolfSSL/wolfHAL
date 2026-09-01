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

/* Board configuration for the NUCLEO-C031C6 dev board */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"
#include <wolfHAL/platform/st/stm32c031xx.h>
#include "wolfHAL_peripheral.h"

/* SysTick timing */
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
    .timeoutTicks = 1000, /* 1s timeout */
    .GetTick = Board_GetTick,
};

static const whal_Stm32c0_Rcc_PeriphClk g_periphClks[] = {
    {WHAL_STM32C031_GPIOA_CLOCK},
    {WHAL_STM32C031_GPIOB_CLOCK},
    {WHAL_STM32C031_GPIOC_CLOCK},
    {WHAL_STM32C031_USART2_CLOCK},
    {WHAL_STM32C031_SPI1_CLOCK},
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))


/* UART */
whal_Uart g_whalUart = {
    .base = WHAL_STM32C031_USART2_BASE,
    /* .driver: direct API mapping */

    .cfg = &(whal_Stm32c0_Uart_Cfg) {
        .timeout = &g_whalTimeout,
        .brr = WHAL_STM32C0_UART_BRR(48000000, 115200),
    },
};

/* SPI */
whal_Spi g_whalSpi = {
    .base = WHAL_STM32C031_SPI1_BASE,
    /* .driver: direct API mapping */

    .cfg = &(whal_Stm32c0_Spi_Cfg) {
        .pclk = 48000000,
        .timeout = &g_whalTimeout,
    },
};

void Board_WaitMs(size_t ms)
{
    uint32_t startCount = g_tick;
    while ((g_tick - startCount) < ms)
        ;
}

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Set flash latency before increasing clock speed */
    err = whal_Stm32c0_Flash_Ext_SetLatency(BOARD_FLASH_DEV, WHAL_STM32C0_FLASH_LATENCY_1);
    if (err)
        return err;

    /* HSI48 / 1 = 48 MHz, then SYSCLK = HSISYS. */
    err = whal_Stm32c0_Rcc_EnableHsi(WHAL_STM32C0_RCC_HSIDIV_1);
    if (err)
        return err;
    err = whal_Stm32c0_Rcc_SetSysClock(WHAL_STM32C0_RCC_SYSCLK_SRC_HSISYS);
    if (err)
        return err;

    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32c0_Rcc_EnablePeriphClk(&g_periphClks[i]);
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
        err = whal_Stm32c0_Rcc_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    return WHAL_SUCCESS;
}
