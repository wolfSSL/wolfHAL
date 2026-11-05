/* stm32wb09ke.h
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

#ifndef WHAL_STM32WB09KE_H
#define WHAL_STM32WB09KE_H

#include <wolfHAL/platform/arm/cortex_m0plus.h>

#include <wolfHAL/clock/stm32wb0_rcc.h>
#include <wolfHAL/gpio/stm32wb0_gpio.h>
#include <wolfHAL/uart/stm32wb0_uart.h>
#include <wolfHAL/spi/stm32wb0_spi.h>
#include <wolfHAL/i2c/stm32wb0_i2c.h>
#include <wolfHAL/flash/stm32wb0_flash.h>
#include <wolfHAL/rng/stm32wb0_rng.h>
#include <wolfHAL/dma/stm32wb0_dma.h>
#include <wolfHAL/watchdog/stm32wb0_iwdg.h>
#include <wolfHAL/crypto/stm32wb0_pka.h>

/**
 * @file stm32wb09ke.h
 * @brief Convenience initializers for STM32WB09KE device instances.
 *
 * Base addresses from RM0505 Table 3 (STM32WB09xE memory map and
 * peripheral register boundary addresses).
 *
 *   GPIOA       0x48000000 (1 KB)
 *   GPIOB       0x48100000 (1 KB), port stride is 1 MB, not 0x400
 *   CRC         0x48200000
 *   PKA slave   0x48300000 (PKA RAM at +0x400)
 *   RCC         0x48400000
 *   PWRC        0x48500000
 *   RNG         0x48600000
 *   DMA slave   0x48700000 (8 channels)
 *   DMAMUX      0x48800000
 *   SYSCFG      0x40000000
 *   FLASH_CTRL  0x40001000
 *   TIM2        0x40002000
 *   IWDG        0x40003000
 *   RTC         0x40004000
 *   TIM16       0x40005000
 *   TIM17       0x40006000
 *   I2C1        0x41000000
 *   USART       0x41004000
 *   LPUART      0x41005000
 *   ADC         0x41006000
 *   SPI3        0x41007000
 *
 * Main flash array is memory-mapped at 0x10040000 (512 KB on WB09KE).
 */

/* --- Base addresses --- */

#define WHAL_STM32WB09_GPIO_BASE        0x48000000
#define WHAL_STM32WB09_CRC_BASE         0x48200000
#define WHAL_STM32WB09_PKA_BASE         0x48300000
#define WHAL_STM32WB09_PWR_BASE         0x48500000
#define WHAL_STM32WB09_RNG_BASE         0x48600000
#define WHAL_STM32WB09_DMA1_BASE        0x48700000
#define WHAL_STM32WB09_DMAMUX1_BASE     0x48800000

#define WHAL_STM32WB09_SYSCFG_BASE      0x40000000
#define WHAL_STM32WB09_FLASH_BASE       0x40001000
#define WHAL_STM32WB09_FLASH_ARRAY_BASE 0x10040000
#define WHAL_STM32WB09_FLASH_SIZE       0x00080000 /* 512 KB on WB09KE */
#define WHAL_STM32WB09_TIM2_BASE        0x40002000
#define WHAL_STM32WB09_IWDG_BASE        0x40003000
#define WHAL_STM32WB09_RTC_BASE         0x40004000
#define WHAL_STM32WB09_TIM16_BASE       0x40005000
#define WHAL_STM32WB09_TIM17_BASE       0x40006000

#define WHAL_STM32WB09_I2C1_BASE        0x41000000
#define WHAL_STM32WB09_USART1_BASE      0x41004000
#define WHAL_STM32WB09_LPUART1_BASE     0x41005000
#define WHAL_STM32WB09_SPI3_BASE        0x41007000

/* --- Vtable-pointer macros ---
 * Only required for drivers whose _DRIVER symbol callers may reference.
 * Flash uses vtable dispatch (lets boards coexist with SPI NOR). PKA uses
 * the crypto peripheral-level vtable.
 */
#define WHAL_STM32WB09_FLASH_DRIVER     &whal_Stm32wb0_Flash_Driver
#define WHAL_STM32WB09_PKA_DRIVER       &whal_Stm32wb0_Pka_CryptoDriver

/* --- Peripheral clock gate descriptors ---
 * RCC base 0x48400000; offsets are register-from-base.
 *   AHBENR  0x50 — GPIO, DMA, CRC, PKA, RNG
 *   APB0ENR 0x54 — TIM2/16/17, RTC, IWDG, SYSCFG
 *   APB1ENR 0x58 — USART, LPUART, SPI3, I2C1, ADC
 *   APB2ENR 0x60 — MR_BLE radio
 */

/* RCC_AHBENR (offset 0x50) */
#define WHAL_STM32WB09_DMA1_CLOCK \
    .regOffset = 0x50,            \
    .enableMask = (1UL << 0),     \
    .enablePos = 0

#define WHAL_STM32WB09_GPIOA_CLOCK \
    .regOffset = 0x50,             \
    .enableMask = (1UL << 2),      \
    .enablePos = 2

#define WHAL_STM32WB09_GPIOB_CLOCK \
    .regOffset = 0x50,             \
    .enableMask = (1UL << 3),      \
    .enablePos = 3

#define WHAL_STM32WB09_CRC_CLOCK   \
    .regOffset = 0x50,             \
    .enableMask = (1UL << 12),     \
    .enablePos = 12

#define WHAL_STM32WB09_PKA_CLOCK   \
    .regOffset = 0x50,             \
    .enableMask = (1UL << 16),     \
    .enablePos = 16

#define WHAL_STM32WB09_RNG_CLOCK   \
    .regOffset = 0x50,             \
    .enableMask = (1UL << 18),     \
    .enablePos = 18

/* RCC_APB0ENR (offset 0x54) */
#define WHAL_STM32WB09_TIM2_CLOCK  \
    .regOffset = 0x54,             \
    .enableMask = (1UL << 0),      \
    .enablePos = 0

#define WHAL_STM32WB09_TIM16_CLOCK \
    .regOffset = 0x54,             \
    .enableMask = (1UL << 1),      \
    .enablePos = 1

#define WHAL_STM32WB09_TIM17_CLOCK \
    .regOffset = 0x54,             \
    .enableMask = (1UL << 2),      \
    .enablePos = 2

#define WHAL_STM32WB09_SYSCFG_CLOCK \
    .regOffset = 0x54,              \
    .enableMask = (1UL << 8),       \
    .enablePos = 8

#define WHAL_STM32WB09_RTC_CLOCK   \
    .regOffset = 0x54,             \
    .enableMask = (1UL << 12),     \
    .enablePos = 12

#define WHAL_STM32WB09_IWDG_CLOCK  \
    .regOffset = 0x54,             \
    .enableMask = (1UL << 14),     \
    .enablePos = 14

/* RCC_APB1ENR (offset 0x58) */
#define WHAL_STM32WB09_ADCDIG_CLOCK \
    .regOffset = 0x58,              \
    .enableMask = (1UL << 4),       \
    .enablePos = 4

#define WHAL_STM32WB09_ADCANA_CLOCK \
    .regOffset = 0x58,              \
    .enableMask = (1UL << 5),       \
    .enablePos = 5

#define WHAL_STM32WB09_LPUART1_CLOCK \
    .regOffset = 0x58,               \
    .enableMask = (1UL << 8),        \
    .enablePos = 8

#define WHAL_STM32WB09_USART1_CLOCK \
    .regOffset = 0x58,              \
    .enableMask = (1UL << 10),      \
    .enablePos = 10

#define WHAL_STM32WB09_SPI3_CLOCK  \
    .regOffset = 0x58,             \
    .enableMask = (1UL << 14),     \
    .enablePos = 14

#define WHAL_STM32WB09_I2C1_CLOCK  \
    .regOffset = 0x58,             \
    .enableMask = (1UL << 21),     \
    .enablePos = 21

#endif /* WHAL_STM32WB09KE_H */
