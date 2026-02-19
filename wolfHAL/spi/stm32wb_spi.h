#ifndef WHAL_STM32WB_SPI_H
#define WHAL_STM32WB_SPI_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/clock/clock.h>

/*
 * @file stm32wb_spi.h
 * @brief STM32WB SPI driver configuration.
 *
 * The STM32WB SPI peripheral provides:
 * - Full-duplex synchronous serial communication
 * - Master and slave modes (this driver supports master only)
 * - Configurable clock polarity and phase (SPI modes 0-3)
 * - Programmable baud rate prescaler (fPCLK/2 to fPCLK/256)
 * - 4 to 16-bit data frame (this driver uses 8-bit)
 * - Software slave management (chip select via GPIO)
 */

/*
 * @brief SPI clock polarity/phase mode selection.
 */
typedef enum {
    WHAL_STM32WB_SPI_MODE_0, /* CPOL=0, CPHA=0 */
    WHAL_STM32WB_SPI_MODE_1, /* CPOL=0, CPHA=1 */
    WHAL_STM32WB_SPI_MODE_2, /* CPOL=1, CPHA=0 */
    WHAL_STM32WB_SPI_MODE_3, /* CPOL=1, CPHA=1 */
} whal_Stm32wbSpi_Mode;

/*
 * @brief SPI device configuration.
 */
typedef struct whal_Stm32wbSpi_Cfg {
    whal_Clock *clkCtrl;  /* Clock controller for SPI peripheral clock */
    const void *clk;      /* Clock descriptor */
} whal_Stm32wbSpi_Cfg;

/*
 * @brief Per-transaction SPI communication parameters.
 */
typedef struct whal_Stm32wbSpi_ComCfg {
    uint32_t mode;       /* SPI mode (WHAL_STM32WB_SPI_MODE_x) */
    uint32_t baud;       /* Baud rate in Hz */
} whal_Stm32wbSpi_ComCfg;

whal_Error WHAL_DRV_FN(Stm32wbSpi, init)(whal_Spi *spiDev);
whal_Error WHAL_DRV_FN(Stm32wbSpi, deinit)(whal_Spi *spiDev);
whal_Error WHAL_DRV_FN(Stm32wbSpi, sendrecv)(whal_Spi *spiDev, void *spiComCfg, const uint8_t *tx,
                               size_t txLen, uint8_t *rx, size_t rxLen);
whal_Error WHAL_DRV_FN(Stm32wbSpi, send)(whal_Spi *spiDev, void *spiComCfg, const uint8_t *data,
                           size_t dataSz);
whal_Error WHAL_DRV_FN(Stm32wbSpi, recv)(whal_Spi *spiDev, void *spiComCfg, uint8_t *data,
                           size_t dataSz);

#endif /* WHAL_STM32WB_SPI_H */
