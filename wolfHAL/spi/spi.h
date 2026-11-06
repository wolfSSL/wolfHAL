/* spi.h
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

#ifndef WHAL_SPI_H
#define WHAL_SPI_H

#include <wolfHAL/error.h>
#include <stdint.h>
#include <stddef.h>

/*
 * @file spi.h
 * @brief Generic SPI abstraction and driver interface.
 */

typedef struct whal_Spi whal_Spi;

/* SPI mode (CPOL/CPHA) */
#define WHAL_SPI_MODE_0 0 /* CPOL=0, CPHA=0 */
#define WHAL_SPI_MODE_1 1 /* CPOL=0, CPHA=1 */
#define WHAL_SPI_MODE_2 2 /* CPOL=1, CPHA=0 */
#define WHAL_SPI_MODE_3 3 /* CPOL=1, CPHA=1 */

/*
 * @brief SPI communication session parameters.
 *
 * Platform-independent configuration applied when starting a communication
 * session via StartCom and kept in effect until EndCom. The platform driver
 * translates these to hardware register values.
 */
typedef struct {
    uint32_t freq;       /* Bus frequency in Hz */
    uint8_t mode;        /* WHAL_SPI_MODE_x */
    uint8_t wordSz;      /* Word size in bits (typically 8) */
    uint8_t dataLines;   /* Number of data lines (1, 2, 4, or 8) */
} whal_Spi_ComCfg;

/*
 * @brief Driver vtable for SPI devices.
 */
typedef struct {
    /* Initialize the SPI hardware. */
    whal_Error (*Init)(whal_Spi *spiDev);
    /* Deinitialize the SPI hardware. */
    whal_Error (*Deinit)(whal_Spi *spiDev);
    /* Begin a communication session with the given parameters. */
    whal_Error (*StartCom)(whal_Spi *spiDev, whal_Spi_ComCfg *comCfg);
    /* End the current communication session. */
    whal_Error (*EndCom)(whal_Spi *spiDev);
    /* Perform a bidirectional transfer. */
    whal_Error (*SendRecv)(whal_Spi *spiDev, const void *tx, size_t txLen, void *rx, size_t rxLen);
} whal_SpiDriver;

/*
 * @brief SPI device instance with a base address, driver vtable, and platform-specific cfg.
 */
struct whal_Spi {
    const uintptr_t base;
    const whal_SpiDriver *driver;
    void *cfg;
};

/*
 * @brief Initializes an SPI device and its driver.
 *
 * @param spiDev Pointer to the SPI instance to initialize.
 *
 * @retval WHAL_SUCCESS Driver-specific init completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Spi_Init(whal_Spi *spiDev);
/*
 * @brief Deinitializes an SPI device and releases resources.
 *
 * @param spiDev Pointer to the SPI instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Driver-specific deinit completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Spi_Deinit(whal_Spi *spiDev);
/*
 * @brief Begin a communication session with the given parameters.
 *
 * Configures the SPI peripheral for the specified mode, frequency,
 * and word size. Must be called before any SendRecv calls.
 *
 * @param spiDev  Pointer to the SPI instance.
 * @param comCfg  Per-session communication parameters.
 *
 * @retval WHAL_SUCCESS Communication session started.
 * @retval WHAL_EINVAL   Null pointer or invalid parameters.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Spi_StartCom(whal_Spi *spiDev, whal_Spi_ComCfg *comCfg);
/*
 * @brief End the current communication session.
 *
 * Disables the SPI peripheral. Must be called after the last
 * SendRecv in a session.
 *
 * @param spiDev Pointer to the SPI instance.
 *
 * @retval WHAL_SUCCESS Communication session ended.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Spi_EndCom(whal_Spi *spiDev);
/*
 * @brief Perform a bidirectional SPI transfer.
 *
 * Clocks max(txLen, rxLen) bytes. When one side is shorter:
 * - tx exhausted: sends 0xFF for remaining clocks.
 * - rx exhausted or NULL: received bytes are discarded.
 * Either tx or rx (or both) may be NULL.
 *
 * @param spiDev Pointer to the SPI instance.
 * @param tx     Buffer of bytes to transmit (NULL to send 0xFF).
 * @param txLen  Number of bytes in tx (0 when tx is NULL).
 * @param rx     Buffer to receive bytes into (NULL to discard).
 * @param rxLen  Number of bytes to receive (0 when rx is NULL).
 *
 * @retval WHAL_SUCCESS Transfer completed.
 * @retval WHAL_EINVAL   Null pointer.
 * @retval WHAL_ENOTSUP Operation not implemented by this driver.
 */
whal_Error whal_Spi_SendRecv(whal_Spi *spiDev, const void *tx, size_t txLen, void *rx, size_t rxLen);

#endif /* WHAL_SPI_H */
