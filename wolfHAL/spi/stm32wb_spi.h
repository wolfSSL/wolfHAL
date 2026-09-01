/* stm32wb_spi.h
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

#ifndef WHAL_STM32WB_SPI_H
#define WHAL_STM32WB_SPI_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/timeout.h>

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
 * @brief SPI device configuration.
 */
typedef struct whal_Stm32wb_Spi_Cfg {
    uint32_t pclk;        /* Peripheral clock frequency in Hz */
    whal_Timeout *timeout;
} whal_Stm32wb_Spi_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_STM32WB_SPI_DEV initializer in wolfHAL_board.h.
 */
#if defined(WHAL_CFG_STM32WB_SPI_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32C0_SPI_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32F0_SPI_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32F3_SPI_SINGLE_INSTANCE) || \
    defined(WHAL_CFG_STM32WB0_SPI_SINGLE_INSTANCE)
extern const whal_Spi whal_Stm32wb_Spi_Dev;
#endif

#if !defined(WHAL_CFG_STM32WB_SPI_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32C0_SPI_DIRECT_API_MAPPING) && \
    !defined(WHAL_CFG_STM32WB0_SPI_DIRECT_API_MAPPING)
/*
 * @brief Driver instance for STM32 SPI peripheral.
 */
extern const whal_SpiDriver whal_Stm32wb_Spi_Driver;

/*
 * @brief Initialize the STM32 SPI peripheral.
 *
 * @param spiDev SPI device instance to initialize.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Spi_Init(whal_Spi *spiDev);

/*
 * @brief Deinitialize the STM32 SPI peripheral.
 *
 * @param spiDev SPI device instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Spi_Deinit(whal_Spi *spiDev);

/*
 * @brief Begin a communication session on the STM32 SPI peripheral.
 *
 * Configures clock polarity, phase, and baud rate from comCfg,
 * then enables the peripheral.
 *
 * @param spiDev  SPI device instance.
 * @param comCfg  Per-session communication parameters.
 *
 * @retval WHAL_SUCCESS Communication session started.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Spi_StartCom(whal_Spi *spiDev, whal_Spi_ComCfg *comCfg);

/*
 * @brief End the current communication session on the STM32 SPI peripheral.
 *
 * Disables SPE.
 *
 * @param spiDev  SPI device instance.
 *
 * @retval WHAL_SUCCESS Communication session ended.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Spi_EndCom(whal_Spi *spiDev);

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
whal_Error whal_Stm32wb_Spi_SendRecv(whal_Spi *spiDev,
                                     const void *tx, size_t txLen,
                                     void *rx, size_t rxLen);
#endif /* !WHAL_CFG_SPI_API_MAPPING */

#endif /* WHAL_STM32WB_SPI_H */
