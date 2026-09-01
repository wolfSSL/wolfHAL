/* main.c
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
#include <wolfHAL/wolfHAL.h>
#include "wolfHAL_board.h"
#include "test.h"

#ifdef WHAL_TEST_ENABLE_CLOCK_PLATFORM
void whal_Test_Clock_Platform(void);
#endif

#ifdef WHAL_TEST_ENABLE_GPIO
void whal_Test_Gpio(void);
#ifdef WHAL_TEST_ENABLE_GPIO_PLATFORM
void whal_Test_Gpio_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_FLASH
void whal_Test_Flash(void);
#ifdef WHAL_TEST_ENABLE_FLASH_PLATFORM
void whal_Test_Flash_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_TIMER
void whal_Test_Timer(void);
#ifdef WHAL_TEST_ENABLE_TIMER_PLATFORM
void whal_Test_Timer_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_RNG
void whal_Test_Rng(void);
#ifdef WHAL_TEST_ENABLE_RNG_PLATFORM
void whal_Test_Rng_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_IPC
void whal_Test_Ipc(void);
#ifdef WHAL_TEST_ENABLE_IPC_PLATFORM
void whal_Test_Ipc_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_SPI_LOOPBACK
void whal_Test_Spi_Loopback(void);
#ifdef WHAL_TEST_ENABLE_SPI_LOOPBACK_PLATFORM
void whal_Test_Spi_Loopback_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_CRYPTO_PLATFORM
void whal_Test_Crypto_Platform(void);
#endif

#ifdef WHAL_TEST_ENABLE_AES_ECB
void whal_Test_AesEcb(void);
#endif

#ifdef WHAL_TEST_ENABLE_AES_CBC
void whal_Test_AesCbc(void);
#endif

#ifdef WHAL_TEST_ENABLE_AES_CTR
void whal_Test_AesCtr(void);
#endif

#ifdef WHAL_TEST_ENABLE_AES_GCM
void whal_Test_AesGcm(void);
#endif

#ifdef WHAL_TEST_ENABLE_AES_GMAC
void whal_Test_AesGmac(void);
#endif

#ifdef WHAL_TEST_ENABLE_AES_CCM
void whal_Test_AesCcm(void);
#endif

#ifdef WHAL_TEST_ENABLE_PKA
void whal_Test_Pka(void);
#endif

#ifdef WHAL_TEST_ENABLE_SHA1
void whal_Test_Sha1(void);
#endif

#ifdef WHAL_TEST_ENABLE_SHA224
void whal_Test_Sha224(void);
#endif

#ifdef WHAL_TEST_ENABLE_SHA256
void whal_Test_Sha256(void);
#endif

#ifdef WHAL_TEST_ENABLE_HMAC_SHA1
void whal_Test_HmacSha1(void);
#endif

#ifdef WHAL_TEST_ENABLE_HMAC_SHA224
void whal_Test_HmacSha224(void);
#endif

#ifdef WHAL_TEST_ENABLE_HMAC_SHA256
void whal_Test_HmacSha256(void);
#endif

#ifdef WHAL_TEST_ENABLE_BLOCK
void whal_Test_Block(void);
#endif

#ifdef WHAL_TEST_ENABLE_ETH
void whal_Test_Eth(void);
#ifdef WHAL_TEST_ENABLE_ETH_PLATFORM
void whal_Test_Eth_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_BMI270_SENSOR
void whal_Test_Bmi270_Sensor(void);
#endif

#ifdef WHAL_TEST_ENABLE_SHARP_MEMORY_DISPLAY
void whal_Test_SharpMemoryDisplay(void);
#endif

#ifdef WHAL_TEST_ENABLE_WATCHDOG
void whal_Test_Watchdog(void);
#ifdef WHAL_TEST_ENABLE_WATCHDOG_PLATFORM
void whal_Test_Watchdog_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_UART
void whal_Test_Uart(void);
#ifdef WHAL_TEST_ENABLE_UART_PLATFORM
void whal_Test_Uart_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_I2C
void whal_Test_I2c(void);
#ifdef WHAL_TEST_ENABLE_I2C_PLATFORM
void whal_Test_I2c_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_PWM
void whal_Test_Pwm(void);
#ifdef WHAL_TEST_ENABLE_PWM_PLATFORM
void whal_Test_Pwm_Platform(void);
#endif
#endif

#ifdef WHAL_TEST_ENABLE_PWM_PULSE
void whal_Test_Pwm_Pulse(void);
#endif

#ifdef WHAL_TEST_ENABLE_DMA_PLATFORM
void whal_Test_Dma_Platform(void);
#endif

#ifdef WHAL_TEST_ENABLE_IRQ_PLATFORM
void whal_Test_Irq_Platform(void);
#endif

int g_whalTestPassed;
int g_whalTestFailed;
int g_whalTestSkipped;
int g_whalTestCurFailed;
int g_whalTestCurSkipped;

void whal_Test_Puts(const char *s)
{
    size_t len = 0;
    while (s[len])
        len++;

    if (len == 0)
        return;

    whal_Uart_Send(BOARD_UART_DEV, s, len);

    if (s[len - 1] == '\n')
        whal_Uart_Send(BOARD_UART_DEV, "\r", 1);
}

void main(void)
{
    g_whalTestPassed = 0;
    g_whalTestFailed = 0;
    g_whalTestSkipped = 0;

    if (Board_Init() != WHAL_SUCCESS)
        while (1);

    whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1);

    whal_Test_Printf("wolfHAL HW Test Suite\n");
    whal_Test_Printf("=====================\n");

#ifdef WHAL_TEST_ENABLE_CLOCK_PLATFORM
    whal_Test_Clock_Platform();
#endif

#ifdef WHAL_TEST_ENABLE_GPIO
    whal_Test_Gpio();
#ifdef WHAL_TEST_ENABLE_GPIO_PLATFORM
    whal_Test_Gpio_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_FLASH
    whal_Test_Flash();
#ifdef WHAL_TEST_ENABLE_FLASH_PLATFORM
    whal_Test_Flash_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_TIMER
    whal_Test_Timer();
#ifdef WHAL_TEST_ENABLE_TIMER_PLATFORM
    whal_Test_Timer_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_RNG
    whal_Test_Rng();
#ifdef WHAL_TEST_ENABLE_RNG_PLATFORM
    whal_Test_Rng_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_IPC
    whal_Test_Ipc();
#ifdef WHAL_TEST_ENABLE_IPC_PLATFORM
    whal_Test_Ipc_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_SPI_LOOPBACK
    whal_Test_Spi_Loopback();
#ifdef WHAL_TEST_ENABLE_SPI_LOOPBACK_PLATFORM
    whal_Test_Spi_Loopback_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_CRYPTO_PLATFORM
    whal_Test_Crypto_Platform();
#endif

#ifdef WHAL_TEST_ENABLE_AES_ECB
    whal_Test_AesEcb();
#endif

#ifdef WHAL_TEST_ENABLE_AES_CBC
    whal_Test_AesCbc();
#endif

#ifdef WHAL_TEST_ENABLE_AES_CTR
    whal_Test_AesCtr();
#endif

#ifdef WHAL_TEST_ENABLE_AES_GCM
    whal_Test_AesGcm();
#endif

#ifdef WHAL_TEST_ENABLE_AES_GMAC
    whal_Test_AesGmac();
#endif

#ifdef WHAL_TEST_ENABLE_AES_CCM
    whal_Test_AesCcm();
#endif

#ifdef WHAL_TEST_ENABLE_PKA
    whal_Test_Pka();
#endif

#ifdef WHAL_TEST_ENABLE_SHA1
    whal_Test_Sha1();
#endif

#ifdef WHAL_TEST_ENABLE_SHA224
    whal_Test_Sha224();
#endif

#ifdef WHAL_TEST_ENABLE_SHA256
    whal_Test_Sha256();
#endif

#ifdef WHAL_TEST_ENABLE_HMAC_SHA1
    whal_Test_HmacSha1();
#endif

#ifdef WHAL_TEST_ENABLE_HMAC_SHA224
    whal_Test_HmacSha224();
#endif

#ifdef WHAL_TEST_ENABLE_HMAC_SHA256
    whal_Test_HmacSha256();
#endif

#ifdef WHAL_TEST_ENABLE_BLOCK
    whal_Test_Block();
#endif

#ifdef WHAL_TEST_ENABLE_ETH
    whal_Test_Eth();
#ifdef WHAL_TEST_ENABLE_ETH_PLATFORM
    whal_Test_Eth_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_BMI270_SENSOR
    whal_Test_Bmi270_Sensor();
#endif

#ifdef WHAL_TEST_ENABLE_SHARP_MEMORY_DISPLAY
    whal_Test_SharpMemoryDisplay();
#endif

#ifdef WHAL_TEST_ENABLE_WATCHDOG
    whal_Test_Watchdog();
#ifdef WHAL_TEST_ENABLE_WATCHDOG_PLATFORM
    whal_Test_Watchdog_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_UART
    whal_Test_Uart();
#ifdef WHAL_TEST_ENABLE_UART_PLATFORM
    whal_Test_Uart_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_I2C
    whal_Test_I2c();
#ifdef WHAL_TEST_ENABLE_I2C_PLATFORM
    whal_Test_I2c_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_PWM
    whal_Test_Pwm();
#ifdef WHAL_TEST_ENABLE_PWM_PLATFORM
    whal_Test_Pwm_Platform();
#endif
#endif

#ifdef WHAL_TEST_ENABLE_PWM_PULSE
    whal_Test_Pwm_Pulse();
#endif

#ifdef WHAL_TEST_ENABLE_DMA_PLATFORM
    whal_Test_Dma_Platform();
#endif

#ifdef WHAL_TEST_ENABLE_IRQ_PLATFORM
    whal_Test_Irq_Platform();
#endif

    WHAL_TEST_SUMMARY();

    if (g_whalTestFailed == 0) {
        whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1);
        while (1);
    }

    while (1) {
        whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1);
        Board_WaitMs(100);
        whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 0);
        Board_WaitMs(100);
    }
}
