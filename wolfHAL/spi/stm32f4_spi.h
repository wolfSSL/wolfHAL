/* stm32f4_spi.h
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

#ifndef WHAL_STM32F4_SPI_H
#define WHAL_STM32F4_SPI_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32f4_spi.h
 * @brief STM32F4 SPI driver configuration.
 *
 * The STM32F4 SPI peripheral provides:
 * - Full-duplex synchronous serial communication
 * - Master and slave modes (this driver supports master only)
 * - Configurable clock polarity and phase (SPI modes 0-3)
 * - Programmable baud rate prescaler (fPCLK/2 to fPCLK/256)
 * - 8 or 16-bit data frame (DFF bit in CR1)
 * - Software slave management (chip select via GPIO)
 *
 * Unlike the STM32WB SPI which has a configurable data size (DS) field
 * in CR2 and FIFO threshold (FRXTH), the STM32F4 uses a single DFF bit
 * in CR1 for 8/16-bit selection.
 */

/*
 * @brief SPI device configuration.
 */
typedef struct whal_Stm32f4_Spi_Cfg {
    uint32_t pclk;        /* Peripheral clock frequency in Hz */
    whal_Timeout *timeout;
} whal_Stm32f4_Spi_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_STM32F4_SPI_DEV initializer in wolfHAL_board.h.
 */
#if defined(WHAL_CFG_STM32F4_SPI_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32L1_SPI_SINGLE_INSTANCE)
extern const whal_Spi whal_Stm32f4_Spi_Dev;
#endif

#ifndef WHAL_CFG_STM32F4_SPI_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32F4 SPI peripheral.
 */
extern const whal_SpiDriver whal_Stm32f4_Spi_Driver;

/*
 * @brief Initialize the STM32F4 SPI peripheral.
 *
 * @param spiDev SPI device instance to initialize.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32f4_Spi_Init(whal_Spi *spiDev);

/*
 * @brief Deinitialize the STM32F4 SPI peripheral.
 *
 * @param spiDev SPI device instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32f4_Spi_Deinit(whal_Spi *spiDev);

/*
 * @brief Begin a communication session on the STM32F4 SPI peripheral.
 *
 * Configures clock polarity, phase, baud rate, and data frame format
 * from comCfg, then enables the peripheral.
 *
 * @param spiDev  SPI device instance.
 * @param comCfg  Per-session communication parameters.
 *
 * @retval WHAL_SUCCESS Communication session started.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32f4_Spi_StartCom(whal_Spi *spiDev, whal_Spi_ComCfg *comCfg);

/*
 * @brief End the current communication session on the STM32F4 SPI peripheral.
 *
 * Disables SPE.
 *
 * @param spiDev  SPI device instance.
 *
 * @retval WHAL_SUCCESS Communication session ended.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32f4_Spi_EndCom(whal_Spi *spiDev);

/*
 * @brief Perform a full-duplex SPI transfer.
 *
 * @param spiDev  SPI device instance.
 * @param tx      Data to transmit (may be NULL).
 * @param txLen   Number of bytes to transmit.
 * @param rx      Receive buffer (may be NULL).
 * @param rxLen   Number of bytes to receive.
 *
 * @retval WHAL_SUCCESS Transfer completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32f4_Spi_SendRecv(whal_Spi *spiDev,
                                     const void *tx, size_t txLen,
                                     void *rx, size_t rxLen);
#endif /* !WHAL_CFG_STM32F4_SPI_DIRECT_API_MAPPING */

#endif /* WHAL_STM32F4_SPI_H */
