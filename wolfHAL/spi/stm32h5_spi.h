/* stm32h5_spi.h
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

#ifndef WHAL_STM32H5_SPI_H
#define WHAL_STM32H5_SPI_H

#include <wolfHAL/spi/spi.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32h5_spi.h
 * @brief STM32H5 SPI driver configuration.
 *
 * The STM32H5 SPI peripheral differs significantly from the STM32WB SPI:
 * - Separate configuration registers (SPI_CFG1, SPI_CFG2) instead of CR1/CR2
 * - Configurable data size up to 32 bits (DSIZE field)
 * - TX/RX FIFOs with configurable threshold (FTHLV)
 * - Transfer size counter (TSIZE in SPI_CR2)
 * - Explicit CSTART bit to begin master transfers
 * - Byte-accessible TXDR/RXDR at different offsets (0x020, 0x030)
 */

/*
 * @brief STM32H5 SPI configuration parameters.
 */
typedef struct whal_Stm32h5_Spi_Cfg {
    size_t pclk;            /* Peripheral clock frequency in Hz */
    whal_Timeout *timeout;
} whal_Stm32h5_Spi_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_STM32H5_SPI_DEV initializer in wolfHAL_board.h.
 */
#if defined(WHAL_CFG_STM32H5_SPI_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32N6_SPI_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32WBA_SPI_SINGLE_INSTANCE)
extern const whal_Spi whal_Stm32h5_Spi_Dev;
#endif

#ifndef WHAL_CFG_STM32H5_SPI_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32H5 SPI peripheral.
 */
extern const whal_SpiDriver whal_Stm32h5_Spi_Driver;

/*
 * @brief Initialize the STM32H5 SPI peripheral.
 *
 * Configures the SPI as master with software slave management.
 *
 * @param spiDev SPI device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Spi_Init(whal_Spi *spiDev);

/*
 * @brief Deinitialize the STM32H5 SPI peripheral.
 *
 * Disables the SPI peripheral.
 *
 * @param spiDev SPI device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Spi_Deinit(whal_Spi *spiDev);

/*
 * @brief Begin a communication session with the given parameters.
 *
 * Configures CPOL, CPHA, baud rate, and data size. Enables the SPI.
 *
 * @param spiDev SPI device instance.
 * @param comCfg Per-session communication parameters.
 *
 * @retval WHAL_SUCCESS Session started.
 * @retval WHAL_EINVAL  Invalid parameters.
 */
whal_Error whal_Stm32h5_Spi_StartCom(whal_Spi *spiDev, whal_Spi_ComCfg *comCfg);

/*
 * @brief End the current communication session.
 *
 * Disables the SPI peripheral.
 *
 * @param spiDev SPI device instance.
 *
 * @retval WHAL_SUCCESS Session ended.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Spi_EndCom(whal_Spi *spiDev);

/*
 * @brief Perform a bidirectional SPI transfer.
 *
 * Clocks max(txLen, rxLen) bytes. Pads TX with 0xFF when exhausted,
 * discards RX when exhausted or NULL.
 *
 * @param spiDev SPI device instance.
 * @param tx     Buffer to transmit (NULL to send 0xFF).
 * @param txLen  Number of bytes to transmit.
 * @param rx     Buffer to receive (NULL to discard).
 * @param rxLen  Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS Transfer completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Spi_SendRecv(whal_Spi *spiDev, const void *tx,
                                     size_t txLen, void *rx, size_t rxLen);
#endif /* !WHAL_CFG_STM32H5_SPI_DIRECT_API_MAPPING */

#endif /* WHAL_STM32H5_SPI_H */
