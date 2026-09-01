/* sdhc_spi_block.h
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

#ifndef WHAL_SDHC_SPI_H
#define WHAL_SDHC_SPI_H

#include <stddef.h>
#include <stdint.h>
#include <wolfHAL/block/block.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/timeout.h>

/*
 * @file sdhc_spi.h
 * @brief SD card driver over SPI (SDHC/SDXC).
 *
 * Implements the whal_Block interface for SD cards using the SD/SPI
 * protocol. Supports SDHC and SDXC cards with 512-byte block addressing.
 *
 * The driver handles:
 * - Card initialization (CMD0, CMD8, ACMD41, CMD58)
 * - Single and multi-block reads (CMD17, CMD18)
 * - Single and multi-block writes (CMD24, CMD25)
 * - Block range erase (CMD32, CMD33, CMD38)
 */

#define WHAL_SDHC_SPI_BLOCK_SZ 512

/*
 * @brief Configuration for the SDHC-over-SPI block driver.
 */
typedef struct whal_SdhcSpi_Cfg {
    whal_Spi *spiDev;           /* SPI bus device */
    whal_Spi_ComCfg *spiComCfg; /* SPI session config for StartCom */
    whal_Gpio *gpioDev;         /* GPIO device for chip select */
    size_t csPin;               /* GPIO pin index for chip select */
    whal_Timeout *timeout;      /* Optional timeout for poll loops */
} whal_SdhcSpi_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_SDHC_SPI_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_SDHC_SPI_SINGLE_INSTANCE
extern const whal_Block whal_SdhcSpi_Dev;
#endif

#ifndef WHAL_CFG_SDHC_SPI_BLOCK_DIRECT_API_MAPPING
/*
 * @brief Driver instance for SDHC/SDXC over SPI.
 */
extern const whal_BlockDriver whal_SdhcSpi_Driver;

/*
 * @brief Initialize the SD card via the SD/SPI protocol (CMD0, CMD8, ACMD41,
 *        CMD58). Caller must have already initialized the SPI bus.
 *
 * @param blockDev Block device instance.
 *
 * @retval WHAL_SUCCESS   Card initialized and ready.
 * @retval WHAL_EINVAL    Null pointer or missing cfg.
 * @retval WHAL_ETIMEOUT  Card did not exit idle within the configured timeout.
 * @retval WHAL_EHARDWARE Card returned an unexpected response or unsupported.
 */
whal_Error whal_SdhcSpi_Init(whal_Block *blockDev);

/*
 * @brief Deinitialize the SD card driver.
 *
 * @param blockDev Block device instance.
 *
 * @retval WHAL_SUCCESS Driver is deinitialized.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_SdhcSpi_Deinit(whal_Block *blockDev);

/*
 * @brief Read `blockCount` 512-byte blocks starting at `block` into `data`
 *        (CMD17 single, CMD18 multi).
 *
 * @param blockDev   Block device instance.
 * @param block      First block index.
 * @param data       Destination buffer (`blockCount * 512` bytes).
 * @param blockCount Number of 512-byte blocks to read.
 *
 * @retval WHAL_SUCCESS   Read completed.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ETIMEOUT  Card did not respond within the configured timeout.
 * @retval WHAL_EHARDWARE CRC or read-error response.
 */
whal_Error whal_SdhcSpi_Read(whal_Block *blockDev, uint32_t block,
                              void *data, uint32_t blockCount);

/*
 * @brief Write `blockCount` 512-byte blocks starting at `block` from `data`
 *        (CMD24 single, CMD25 multi).
 *
 * @param blockDev   Block device instance.
 * @param block      First block index.
 * @param data       Source buffer (`blockCount * 512` bytes).
 * @param blockCount Number of 512-byte blocks to write.
 *
 * @retval WHAL_SUCCESS   Write completed.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ETIMEOUT  Card did not respond within the configured timeout.
 * @retval WHAL_EHARDWARE Programming error response.
 */
whal_Error whal_SdhcSpi_Write(whal_Block *blockDev, uint32_t block,
                               const void *data, uint32_t blockCount);

/*
 * @brief Erase a range of blocks via CMD32/CMD33/CMD38.
 *
 * @param blockDev   Block device instance.
 * @param block      First block index.
 * @param blockCount Number of 512-byte blocks to erase.
 *
 * @retval WHAL_SUCCESS   Erase completed.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ETIMEOUT  Erase did not complete within the configured timeout.
 * @retval WHAL_EHARDWARE Erase error response.
 */
whal_Error whal_SdhcSpi_Erase(whal_Block *blockDev, uint32_t block,
                               uint32_t blockCount);
#endif /* !WHAL_CFG_SDHC_SPI_BLOCK_DIRECT_API_MAPPING */

#endif /* WHAL_SDHC_SPI_H */
