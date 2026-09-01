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

/* Board configuration for the STM32WBA55CG Nucleo (NUCLEO-WBA55CG) */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"
#include <wolfHAL/platform/st/stm32wba55cg.h>
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

/* Clock: PLL1 from HSE32, targeting 100 MHz
 *   HSE32 = 32 MHz
 *   PLL1M = 1 (div 2) -> ref_ck = 16 MHz
 *   PLL1N = 25 -> VCO = 16 * 25 = 400 MHz
 *   PLL1R = 3 (div 4) -> pll1rclk = 100 MHz
 *   PLL1RGE = 3 (8-16 MHz range)
 */
static const whal_Stm32wba_Rcc_PeriphClk g_flashClock = {WHAL_STM32WBA55_FLASH_CLOCK};

static const whal_Stm32wba_Rcc_PeriphClk g_periphClks[] = {
    {WHAL_STM32WBA55_GPIOA_CLOCK},
    {WHAL_STM32WBA55_GPIOB_CLOCK},
    {WHAL_STM32WBA55_GPIOC_CLOCK},
    {WHAL_STM32WBA55_USART1_CLOCK},
    {WHAL_STM32WBA55_SPI1_CLOCK},
    {WHAL_STM32WBA55_RNG_CLOCK},
    {WHAL_STM32WBA55_AES_CLOCK},
    {WHAL_STM32WBA55_HASH_CLOCK},
    {WHAL_STM32WBA55_I2C1_CLOCK},
#ifdef BOARD_WATCHDOG_WWDG
    {WHAL_STM32WBA55_WWDG_CLOCK},
#endif
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))


/* I2C */
whal_I2c g_whalI2c = {
    .base = WHAL_STM32WBA55_I2C1_BASE,
    .driver = WHAL_STM32WBA55_I2C1_DRIVER,

    .cfg = &(whal_Stm32wba_I2c_Cfg) {
        .pclk = 100000000,
        .timeout = &g_whalTimeout,
    },
};

/* SPI */
whal_Spi g_whalSpi = {
    .base = WHAL_STM32WBA55_SPI1_BASE,
    .driver = WHAL_STM32WBA55_SPI1_DRIVER,

    .cfg = &(whal_Stm32wba_Spi_Cfg) {
        .pclk = 100000000,
        .timeout = &g_whalTimeout,
    },
};

/* DMA */
#ifdef BOARD_DMA

whal_Dma g_whalDma1 = {
    .base = WHAL_STM32WBA55_GPDMA1_BASE,
    .driver = WHAL_STM32WBA55_GPDMA1_DRIVER,
    .cfg = &(whal_Stm32wba_Gpdma_Cfg){
        .numChannels = 8,
        .timeout = &g_whalTimeout,
    },
};

static const whal_Stm32wba_Rcc_PeriphClk g_dmaClock = {WHAL_STM32WBA55_GPDMA1_CLOCK};

void GPDMA1_Channel0_IRQHandler(void)
{
    whal_Stm32wba_Gpdma_IRQHandler(&g_whalDma1, 0,
                                   whal_Stm32wba_UartDma_TxCallback,
                                   g_whalUart.cfg);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    whal_Stm32wba_Gpdma_IRQHandler(&g_whalDma1, 1,
                                   whal_Stm32wba_UartDma_RxCallback,
                                   g_whalUart.cfg);
}
#endif

/* UART (USART1 via VCP at 115200 baud) */
#ifdef BOARD_DMA
whal_Uart g_whalUart = {
    .base = WHAL_STM32WBA55_USART1_BASE,
    .driver = &whal_Stm32wba_UartDma_Driver,
    .cfg = &(whal_Stm32wba_UartDma_Cfg) {
        .base = {
            .brr = WHAL_STM32WBA_UART_BRR(100000000, 115200),
            .timeout = &g_whalTimeout,
        },
        .dma = &g_whalDma1,
        .txCh = 0,
        .rxCh = 1,
        .txChCfg = &(whal_Stm32wba_Gpdma_ChCfg){WHAL_STM32WBA55_USART1_TX_DMA_CFG},
        .rxChCfg = &(whal_Stm32wba_Gpdma_ChCfg){WHAL_STM32WBA55_USART1_RX_DMA_CFG},
    },
};
#else
whal_Uart g_whalUart = {
    .base = WHAL_STM32WBA55_USART1_BASE,
    .driver = WHAL_STM32WBA55_USART1_DRIVER,

    .cfg = &(whal_Stm32wba_Uart_Cfg) {
        .timeout = &g_whalTimeout,
        .brr = WHAL_STM32WBA_UART_BRR(100000000, 115200),
    },
};
#endif

/* RNG, AES + mode, HASH + algorithm singletons are defined in their driver TUs
 * from WHAL_CFG_* initializers in wolfHAL_board.h. */

/* Hash (HASH hardware accelerator) — vtable dispatcher for whal_Crypto_Init/Deinit. */
whal_Crypto g_whalHash = {
    .base = WHAL_STM32WBA55_HASH_BASE,
    .driver = &whal_Stm32wba_Hash_CryptoDriver,

    .cfg = &(whal_Stm32wba_Hash_Cfg) {
        .timeout = &g_whalTimeout,
    },
};

#ifdef BOARD_WATCHDOG_IWDG
whal_Watchdog g_whalWatchdog = {
    .base = WHAL_STM32WBA55_IWDG_BASE,
    .driver = WHAL_STM32WBA55_IWDG_DRIVER,

    .cfg = &(whal_Stm32wba_Iwdg_Cfg) {
        .prescaler = WHAL_STM32WBA_IWDG_PR_32,
        .reload = 100,
        .timeout = &g_whalTimeout,
    },
};
#elif defined(BOARD_WATCHDOG_WWDG)
whal_Watchdog g_whalWatchdog = {
    .base = WHAL_STM32WBA55_WWDG_BASE,
    .driver = WHAL_STM32WBA55_WWDG_DRIVER,

    .cfg = &(whal_Stm32wba_Wwdg_Cfg) {
        .prescaler = WHAL_STM32WBA_WWDG_TB_128,
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

/*
 * Switch PWR voltage scaling to Range 1 (required for >16 MHz operation).
 * After reset the device is in Range 2 (max 16 MHz). Must switch to Range 1
 * before configuring PLL or increasing SYSCLK.
 *
 * PWR base: 0x46020800
 * PWR_VOSR offset: 0x00C
 *   bit 16 VOS: 0=Range2, 1=Range1
 *   bit 15 VOSRDY: read-only, 1 when stable
 */
#define PWR_BASE       0x46020800
#define PWR_VOSR_REG   0x00C
#define PWR_VOSR_VOS_Msk    (1UL << 16)
#define PWR_VOSR_VOSRDY_Msk (1UL << 15)

static whal_Error Board_SetVosRange1(void)
{
    /* Set VOS to Range 1 */
    whal_Reg_Update(PWR_BASE, PWR_VOSR_REG, PWR_VOSR_VOS_Msk, PWR_VOSR_VOS_Msk);

    /* Wait for VOSRDY */
    while (!(whal_Reg_Read(PWR_BASE, PWR_VOSR_REG) & PWR_VOSR_VOSRDY_Msk))
        ;

    return WHAL_SUCCESS;
}

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Enable PWR clock (RCC_AHB4ENR bit 2) -- required to access PWR registers */
    static const whal_Stm32wba_Rcc_PeriphClk pwrClock = {WHAL_STM32WBA55_PWR_CLOCK};
    err = whal_Stm32wba_Rcc_EnablePeriphClk(&pwrClock);
    if (err)
        return err;

    /* Switch to voltage Range 1 (required for >16 MHz operation).
     * After reset device is in Range 2 which caps SYSCLK at 16 MHz. */
    err = Board_SetVosRange1();
    if (err)
        return err;

    /* Enable flash clock and set latency before increasing clock speed.
     * At 100 MHz, 3.3V: 3 wait states required (RM0493 Table 69). */
    err = whal_Stm32wba_Rcc_EnablePeriphClk(&g_flashClock);
    if (err)
        return err;

    err = whal_Stm32wba_Flash_Ext_SetLatency(BOARD_FLASH_DEV, 3);
    if (err)
        return err;

    /* Set HPRE5 to div 4 before switching to PLL (AHB5 max 32 MHz).
     * 100 MHz / 4 = 25 MHz. Value 0b101 = div 4. */
    err = whal_Stm32wba_Rcc_SetHpre5(5);
    if (err)
        return err;

    /* M/N/R are (value-1) encoded per RM0493 (divide/multiply = field + 1):
     *   f_ref  = HSE32 / (M+1) = 32 / 2  = 16 MHz  (PLL1RGE 8-16)
     *   f_vco  = f_ref * (N+1) = 16 * 25 = 400 MHz (VCO 128-544)
     *   SYSCLK = f_vco / (R+1) = 400 / 4 = 100 MHz (part max) */
    err = whal_Stm32wba_Rcc_EnableOsc(
        &(whal_Stm32wba_Rcc_OscCfg){WHAL_STM32WBA_RCC_HSE32_CFG});
    if (err)
        return err;
    err = whal_Stm32wba_Rcc_EnablePll1(&(whal_Stm32wba_Rcc_Pll1Cfg){
        .clkSrc = WHAL_STM32WBA_RCC_PLL1SRC_HSE32,
        .rge = WHAL_STM32WBA_RCC_PLL1RGE_8_16,
        .m = 1, .n = 24, .r = 3, .q = 0, .p = 0,
    });
    if (err)
        return err;
    err = whal_Stm32wba_Rcc_SetSysClock(WHAL_STM32WBA_RCC_SYSCLK_SRC_PLL1);
    if (err)
        return err;

#ifdef BOARD_WATCHDOG_IWDG
    /* Enable LSI oscillator required by IWDG */
    err = whal_Stm32wba_Rcc_EnableLsi();
    if (err)
        return err;
#endif

    /* Select HSI16 as RNG kernel clock source.
     * Default after reset is LSE which is not enabled. HSI16 is always on. */
    err = whal_Stm32wba_Rcc_SetRngClockSrc(WHAL_STM32WBA_RCC_RNGSEL_HSI16);
    if (err)
        return err;

    /* Enable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32wba_Rcc_EnablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    err = whal_Irq_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

#ifdef BOARD_DMA
    err = whal_Stm32wba_Rcc_EnablePeriphClk(&g_dmaClock);
    if (err)
        return err;
    err = whal_Dma_Init(&g_whalDma1);
    if (err)
        return err;

    /* Enable NVIC interrupts for GPDMA1 channel 0 (IRQ 29) and channel 1 (IRQ 30) */
    err = whal_Irq_Enable(WHAL_INTERNAL_DEV, 29, NULL);
    if (err)
        return err;
    err = whal_Irq_Enable(WHAL_INTERNAL_DEV, 30, NULL);
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
    whal_Irq_Disable(WHAL_INTERNAL_DEV, 29);
    whal_Irq_Disable(WHAL_INTERNAL_DEV, 30);

    err = whal_Dma_Deinit(&g_whalDma1);
    if (err)
        return err;
    err = whal_Stm32wba_Rcc_DisablePeriphClk(&g_dmaClock);
    if (err)
        return err;
#endif

    err = whal_Irq_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    /* Disable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32wba_Rcc_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

#ifdef BOARD_WATCHDOG_IWDG
    err = whal_Stm32wba_Rcc_DisableLsi();
    if (err)
        return err;
#endif

    err = whal_Stm32wba_Rcc_SetSysClock(WHAL_STM32WBA_RCC_SYSCLK_SRC_HSI16);
    if (err)
        return err;
    err = whal_Stm32wba_Rcc_DisablePll1();
    if (err)
        return err;
    err = whal_Stm32wba_Rcc_DisableOsc(
        &(whal_Stm32wba_Rcc_OscCfg){WHAL_STM32WBA_RCC_HSE32_CFG});
    if (err)
        return err;

    err = whal_Stm32wba_Flash_Ext_SetLatency(BOARD_FLASH_DEV, 1);
    if (err)
        return err;

    return WHAL_SUCCESS;
}
