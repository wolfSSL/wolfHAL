#ifndef STM32WB55XX_NUCLEO_H
#define STM32WB55XX_NUCLEO_H

#include <wolfHAL/platform/st/stm32wb55xx.h>

/*
 * @file stm32wb55xx_nucleo.h
 * @brief Board-specific handles for the STM32WB55xx Nucleo example.
 */

/* Friendly pin index mapping for the example board. */
enum {
    LED_PIN,
    LPUART1_TX_PIN,
    LPUART1_RX_PIN,
    SPI1_SCK_PIN,
    SPI1_MISO_PIN,
    SPI1_MOSI_PIN,
    CS_PIN,
};

WHAL_CLOCK_DEV_DECLARE(clock, Stm32wbRccPll)
WHAL_GPIO_DEV_DECLARE(gpio, Stm32wbGpio)
WHAL_TIMER_DEV_DECLARE(timer, SysTick)
WHAL_UART_DEV_DECLARE(uart, Stm32wbUart)
WHAL_FLASH_DEV_DECLARE(flash, Stm32wbFlash)
WHAL_SPI_DEV_DECLARE(spi, Stm32wbSpi)
WHAL_RNG_DEV_DECLARE(rng, Stm32wbRng)

#endif /* STM32WB55XX_NUCLEO_H */
