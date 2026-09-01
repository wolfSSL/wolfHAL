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

/* Example board configuration for the STM32WB55 Nucleo dev board */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"
#include <wolfHAL/platform/st/stm32wb55xx.h>
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

static const whal_Stm32wb_Rcc_PeriphClk g_periphClks[] = {
    {WHAL_STM32WB55_GPIOA_GATE},
    {WHAL_STM32WB55_GPIOB_GATE},
    {WHAL_STM32WB55_UART1_GATE},
    {WHAL_STM32WB55_SPI1_GATE},
    {WHAL_STM32WB55_RNG_GATE},
    {WHAL_STM32WB55_AES1_GATE},
    {WHAL_STM32WB55_PKA_GATE},
    {WHAL_STM32WB55_I2C1_GATE},
    {WHAL_STM32WB55_LPTIM1_GATE},
#ifdef BOARD_WATCHDOG_WWDG
    {WHAL_STM32WB55_WWDG_GATE},
#endif
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))

/* I2C */
whal_I2c g_whalI2c = {
    .base = WHAL_STM32WB55_I2C1_BASE,
    /* .driver: direct API mapping */

    .cfg = &(whal_Stm32wb_I2c_Cfg) {
        .pclk = 64000000,
        .timeout = &g_whalTimeout,
    },
};

/* PWM (LPTIM1): vtable dispatch; /128 prescaler makes one PWM tick = 2 us. */
whal_Pwm g_whalPwm = {
    .base = WHAL_STM32WB55_LPTIM1_BASE,
    .driver = WHAL_STM32WB55_LPTIM1_PWM_DRIVER,
    .cfg = &(whal_Stm32wb_Lptim_Pwm_Cfg) {
        .prescaler = 7, /* /128 */
        .clkSel = WHAL_STM32WB_LPTIM_PWM_CLKSEL_INTERNAL,
        .timeout = &g_whalTimeout,
    },
};

/* SPI */
whal_Spi g_whalSpi = {
    .base = WHAL_STM32WB55_SPI1_BASE,
    /* .driver: direct API mapping */

    .cfg = &(whal_Stm32wb_Spi_Cfg) {
        .pclk = 64000000,
        .timeout = &g_whalTimeout,
    },
};

/* DMA */
#ifdef BOARD_DMA

whal_Dma g_whalDma1 = {
    .base = WHAL_STM32WB55_DMA1_BASE,
    /* .driver: direct API mapping */
    .cfg = &(whal_Stm32wb_Dma_Cfg){WHAL_STM32WB55_DMA1_CFG},
};

void DMA1_Channel4_IRQHandler(void)
{
    whal_Stm32wb_Dma_IRQHandler(&g_whalDma1, 3,
                                whal_Stm32wb_UartDma_TxCallback, g_whalUart.cfg);
}

void DMA1_Channel5_IRQHandler(void)
{
    whal_Stm32wb_Dma_IRQHandler(&g_whalDma1, 4,
                                whal_Stm32wb_UartDma_RxCallback, g_whalUart.cfg);
}
#endif

/* UART */
#ifdef BOARD_DMA
whal_Uart g_whalUart = {
    .base = WHAL_STM32WB55_UART1_BASE,
    /* .driver: direct API mapping */
    .cfg = &(whal_Stm32wb_UartDma_Cfg) {
        .base = {
            .brr = WHAL_STM32WB_UART_BRR(64000000, 115200),
            .timeout = &g_whalTimeout,
        },
        .dma = &g_whalDma1,
        .txCh = 3,
        .rxCh = 4,
        .txChCfg = &(whal_Stm32wb_Dma_ChCfg){WHAL_STM32WB55_UART1_TX_DMA_CFG},
        .rxChCfg = &(whal_Stm32wb_Dma_ChCfg){WHAL_STM32WB55_UART1_RX_DMA_CFG},
    },
};
#else
whal_Uart g_whalUart = {
    .base = WHAL_STM32WB55_UART1_BASE,
    /* .driver: direct API mapping */

    .cfg = &(whal_Stm32wb_Uart_Cfg) {
        .timeout = &g_whalTimeout,

        .brr = WHAL_STM32WB_UART_BRR(64000000, 115200),
    },
};
#endif

/* Crypto */
whal_Crypto g_whalCrypto = {
    .base = WHAL_STM32WB55_AES1_BASE,
    /* .driver: direct API mapping */
    .cfg = &(whal_Stm32wb_Aes_Cfg) {
        .timeout = &g_whalTimeout,
    },
};

void Board_WaitMs(size_t ms)
{
    uint32_t startCount = g_tick;
    while (g_tick - startCount < ms);
}

whal_Error Board_Init(void)
{
    whal_Error err;
    size_t i;

    /* Flash latency must be set before SYSCLK rises above ~16 MHz. */
    err = whal_Stm32wb_Flash_Ext_SetLatency(BOARD_FLASH_DEV, WHAL_STM32WB_FLASH_LATENCY_3);
    if (err)
        return err;

    /* Bring up the clock tree: MSI -> PLL (sourced from MSI) -> HSI48 -> LSI -> SYSCLK->PLL */
    err = whal_Stm32wb_Rcc_EnableMsi(WHAL_STM32WB_RCC_MSIRANGE_4MHz);
    if (err)
        return err;

    /* MSI (4 MHz) -> VCO 128 MHz -> PLLR /2 = 64 MHz */
    err = whal_Stm32wb_Rcc_EnablePll(&(whal_Stm32wb_Rcc_PllCfg){
        .clkSrc = WHAL_STM32WB_RCC_PLLCLK_SRC_MSI,
        .n = 32, .m = 0, .r = 1, .q = 0, .p = 0,
    });
    if (err)
        return err;

    err = whal_Stm32wb_Rcc_EnableOsc(
        &(whal_Stm32wb_Rcc_OscCfg){WHAL_STM32WB_RCC_HSI48_CFG});
    if (err)
        return err;

#ifdef BOARD_WATCHDOG_IWDG
    err = whal_Stm32wb_Rcc_EnableOsc(
        &(whal_Stm32wb_Rcc_OscCfg){WHAL_STM32WB_RCC_LSI_CFG});
    if (err)
        return err;
#endif

    err = whal_Stm32wb_Rcc_SetSysClock(WHAL_STM32WB_RCC_SYSCLK_SRC_PLL);
    if (err)
        return err;

    for (i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32wb_Rcc_EnablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    err = whal_Irq_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

#ifdef BOARD_DMA
    err = whal_Stm32wb_Rcc_EnablePeriphClk(&(whal_Stm32wb_Rcc_PeriphClk){WHAL_STM32WB55_DMA1_GATE});
    if (err)
        return err;

    err = whal_Stm32wb_Rcc_EnablePeriphClk(&(whal_Stm32wb_Rcc_PeriphClk){WHAL_STM32WB55_DMAMUX1_GATE});
    if (err)
        return err;

    err = whal_Dma_Init(&g_whalDma1);
    if (err)
        return err;

    /* Enable NVIC interrupts for DMA1 channel 4 (IRQ 14) and channel 5 (IRQ 15) */
    err = whal_Irq_Enable(WHAL_INTERNAL_DEV, 14, NULL);
    if (err)
        return err;

    err = whal_Irq_Enable(WHAL_INTERNAL_DEV, 15, NULL);
    if (err)
        return err;
#endif

    err = whal_Gpio_Init(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Uart_Init(&g_whalUart);
    if (err) {
        return err;
    }

    err = whal_Spi_Init(&g_whalSpi);
    if (err) {
        return err;
    }

    err = whal_I2c_Init(&g_whalI2c);
    if (err) {
        return err;
    }

    err = whal_Pwm_Init(&g_whalPwm);
    if (err) {
        return err;
    }

    err = whal_Flash_Init(BOARD_FLASH_DEV);
    if (err) {
        return err;
    }

    err = whal_Rng_Init(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Crypto_Init(&g_whalCrypto);
    if (err) {
        return err;
    }

    /* PKA: platform-specific Init because whal_Crypto_Init is direct-mapped
     * to whal_Stm32wb_Aes_Init on this board (AES claims the generic name). */
    err = whal_Stm32wb_Pka_Init(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Timer_Init(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Timer_Start(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = Peripheral_Init();
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error Board_Deinit(void)
{
    whal_Error err;

    err = Peripheral_Deinit();
    if (err) {
        return err;
    }

    err = whal_Timer_Stop(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Timer_Deinit(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }


    err = whal_Stm32wb_Pka_Deinit(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Crypto_Deinit(&g_whalCrypto);
    if (err) {
        return err;
    }

    err = whal_Rng_Deinit(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Flash_Deinit(BOARD_FLASH_DEV);
    if (err) {
        return err;
    }

    err = whal_Pwm_Deinit(&g_whalPwm);
    if (err) {
        return err;
    }

    err = whal_I2c_Deinit(&g_whalI2c);
    if (err) {
        return err;
    }

    err = whal_Spi_Deinit(&g_whalSpi);
    if (err) {
        return err;
    }

    err = whal_Uart_Deinit(&g_whalUart);
    if (err) {
        return err;
    }

    err = whal_Gpio_Deinit(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

#ifdef BOARD_DMA
    whal_Irq_Disable(WHAL_INTERNAL_DEV, 14);
    whal_Irq_Disable(WHAL_INTERNAL_DEV, 15);

    err = whal_Dma_Deinit(&g_whalDma1);
    if (err)
        return err;
    err = whal_Stm32wb_Rcc_DisablePeriphClk(&(whal_Stm32wb_Rcc_PeriphClk){WHAL_STM32WB55_DMAMUX1_GATE});
    if (err)
        return err;
    err = whal_Stm32wb_Rcc_DisablePeriphClk(&(whal_Stm32wb_Rcc_PeriphClk){WHAL_STM32WB55_DMA1_GATE});
    if (err)
        return err;
#endif

    err = whal_Irq_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    /* Tear down the clock tree in reverse: peripheral gates off, SYSCLK -> MSI, sources off. */
    for (size_t i = PERIPH_CLK_COUNT; i-- > 0; ) {
        err = whal_Stm32wb_Rcc_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }
    err = whal_Stm32wb_Rcc_SetSysClock(WHAL_STM32WB_RCC_SYSCLK_SRC_MSI);
    if (err)
        return err;
    err = whal_Stm32wb_Rcc_DisablePll();
    if (err)
        return err;
#ifdef BOARD_WATCHDOG_IWDG
    err = whal_Stm32wb_Rcc_DisableOsc(
        &(whal_Stm32wb_Rcc_OscCfg){WHAL_STM32WB_RCC_LSI_CFG});
    if (err)
        return err;
#endif
    err = whal_Stm32wb_Rcc_DisableOsc(
        &(whal_Stm32wb_Rcc_OscCfg){WHAL_STM32WB_RCC_HSI48_CFG});
    if (err)
        return err;
    /* MSI stays on as the post-Deinit fallback. */

    err = whal_Stm32wb_Flash_Ext_SetLatency(BOARD_FLASH_DEV, WHAL_STM32WB_FLASH_LATENCY_0);
    if (err)
        return err;

    return WHAL_SUCCESS;
}
