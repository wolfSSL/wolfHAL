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

/* Board configuration for the NUCLEO-WB05KZ dev board (UM3343 / MB1801+MB2032). */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"
#include <wolfHAL/platform/st/stm32wb05kz.h>

/* Target SYSCLK 64 MHz from the HSI64M tree (PLL-locked onto the 32 MHz
 * HSE crystal). Flash needs 1 wait state above 32 MHz. */
#define BOARD_SYSCLK_HZ 64000000

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
    .timeoutTicks = 1000, /* 1 s timeout */
    .GetTick = Board_GetTick,
};

static const whal_Stm32wb0_Rcc_PeriphClk g_periphClks[] = {
    {WHAL_STM32WB05_GPIOA_CLOCK},
    {WHAL_STM32WB05_GPIOB_CLOCK},
    {WHAL_STM32WB05_USART1_CLOCK},
    {WHAL_STM32WB05_SPI3_CLOCK},
    {WHAL_STM32WB05_I2C1_CLOCK},
    {WHAL_STM32WB05_PKA_CLOCK},
    {WHAL_STM32WB05_SYSCFG_CLOCK},
    {WHAL_STM32WB05_RNG_CLOCK},
#ifdef BOARD_WATCHDOG_IWDG
    {WHAL_STM32WB05_IWDG_CLOCK},
#endif
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))

/* UART (USART via VCP at 115200 baud).
 * BRR uses APB1 clock = SYSCLK on WB0. */
whal_Uart g_whalUart = {
    .base = WHAL_STM32WB05_USART1_BASE,
    /* .driver: direct API mapping */
    .cfg = &(whal_Stm32wb0_Uart_Cfg) {
        .timeout = &g_whalTimeout,
        .brr = WHAL_STM32WB0_UART_BRR(16000000, 115200),
    },
};

/* SPI3 */
whal_Spi g_whalSpi = {
    .base = WHAL_STM32WB05_SPI3_BASE,
    /* .driver: direct API mapping */
    .cfg = &(whal_Stm32wb0_Spi_Cfg) {
        .pclk = BOARD_SYSCLK_HZ,
        .timeout = &g_whalTimeout,
    },
};

/* I2C1 */
whal_I2c g_whalI2c = {
    .base = WHAL_STM32WB05_I2C1_BASE,
    /* .driver: direct API mapping */
    .cfg = &(whal_Stm32wb0_I2c_Cfg) {
        .pclk = BOARD_SYSCLK_HZ,
        .timeout = &g_whalTimeout,
    },
};

/* PKA (public-key accelerator) — vtable dispatcher for whal_Crypto_Init/Deinit. */
whal_Crypto g_whalPka = {
    .base   = WHAL_STM32WB05_PKA_BASE,
    .driver = WHAL_STM32WB05_PKA_DRIVER,
    .cfg    = &(whal_Stm32wb0_Pka_Cfg) {
        .timeout = &g_whalTimeout,
    },
};

#ifdef BOARD_WATCHDOG_IWDG
whal_Watchdog g_whalWatchdog = {
    .base   = WHAL_STM32WB05_IWDG_BASE,
    /* .driver: direct API mapping */
    .cfg    = &(whal_Stm32wb0_Iwdg_Cfg) {
        .prescaler = WHAL_STM32WB0_IWDG_PR_32,
        .reload    = 100,
        .timeout   = &g_whalTimeout,
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

    /* Bring up flash latency BEFORE raising SYSCLK above 32 MHz.
     * RM0529 9.4.2: 1 wait state for sysclk == 64 MHz. */
    err = whal_Stm32wb0_Flash_Ext_SetLatency(BOARD_FLASH_DEV,
                                             WHAL_STM32WB0_FLASH_LATENCY_1);
    if (err)
        return err;

    /* HSI64M is on at reset; defensively poll its ready bit. */
    err = whal_Stm32wb0_Rcc_EnableHsi();
    if (err)
        return err;

    /* Enable the 32 MHz HSE crystal and lock the RC64MPLL onto it.
     * The Nucleo board ships an X2 32 MHz crystal so HSE is the right
     * PLL reference; running the PLL locked yields a clean 64 MHz tree. */
    err = whal_Stm32wb0_Rcc_EnableHse();
    if (err)
        return err;
    err = whal_Stm32wb0_Rcc_EnableHsiPll();
    if (err)
        return err;

    /* Source the fast clock tree from the PLL-locked RC64M, then divide
     * by 1 to land at 64 MHz SYSCLK. */
    err = whal_Stm32wb0_Rcc_SetFastClkSrc(WHAL_STM32WB0_RCC_FASTCLK_RC64MPLL);
    if (err)
        return err;
    err = whal_Stm32wb0_Rcc_SetSysClock(WHAL_STM32WB0_RCC_SYSCLK_64MHZ);
    if (err)
        return err;

#ifdef BOARD_WATCHDOG_IWDG
    /* The IWDG is clocked from LSI (~32 kHz). */
    err = whal_Stm32wb0_Rcc_EnableLsi();
    if (err)
        return err;
#endif

    /* Enable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32wb0_Rcc_EnablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    err = whal_Gpio_Init(BOARD_GPIO_DEV);
    if (err)
        return err;

    err = whal_Uart_Init(BOARD_UART_DEV);
    if (err)
        return err;

    err = whal_Spi_Init(BOARD_SPI_DEV);
    if (err)
        return err;

    err = whal_I2c_Init(BOARD_I2C_DEV);
    if (err)
        return err;

    err = whal_Flash_Init(BOARD_FLASH_DEV);
    if (err)
        return err;

    err = whal_Timer_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Timer_Start(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Rng_Init(BOARD_RNG_DEV);
    if (err)
        return err;

    err = whal_Crypto_Init(BOARD_PKA_DEV);
    if (err)
        return err;


    return WHAL_SUCCESS;
}

whal_Error Board_Deinit(void)
{
    whal_Error err;

    err = whal_Timer_Stop(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Timer_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Crypto_Deinit(BOARD_PKA_DEV);
    if (err)
        return err;

    err = whal_Rng_Deinit(BOARD_RNG_DEV);
    if (err)
        return err;

    err = whal_Flash_Deinit(BOARD_FLASH_DEV);
    if (err)
        return err;

    err = whal_I2c_Deinit(BOARD_I2C_DEV);
    if (err)
        return err;

    err = whal_Spi_Deinit(BOARD_SPI_DEV);
    if (err)
        return err;

    err = whal_Uart_Deinit(BOARD_UART_DEV);
    if (err)
        return err;

    err = whal_Gpio_Deinit(BOARD_GPIO_DEV);
    if (err)
        return err;

    /* Disable peripheral clocks */
    for (size_t i = PERIPH_CLK_COUNT; i-- > 0; ) {
        err = whal_Stm32wb0_Rcc_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

#ifdef BOARD_WATCHDOG_IWDG
    err = whal_Stm32wb0_Rcc_DisableLsi();
    if (err)
        return err;
#endif

    /* Drop back to the default 16 MHz sysclk before dropping flash latency. */
    err = whal_Stm32wb0_Rcc_SetSysClock(WHAL_STM32WB0_RCC_SYSCLK_16MHZ);
    if (err)
        return err;

    err = whal_Stm32wb0_Flash_Ext_SetLatency(BOARD_FLASH_DEV,
                                             WHAL_STM32WB0_FLASH_LATENCY_0);
    if (err)
        return err;

    return WHAL_SUCCESS;
}
