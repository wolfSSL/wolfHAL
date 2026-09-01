/* spi_nor_flash.h
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

#ifndef WHAL_SPI_NOR_H
#define WHAL_SPI_NOR_H

#include <stddef.h>
#include <stdint.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/timeout.h>

/*
 * @file spi_nor.h
 * @brief SPI-NOR flash driver.
 *
 * Implements the whal_Flash interface for SPI-NOR flash devices using
 * standard JEDEC commands.
 *
 * The driver provides composable function variants for four addressing modes:
 *
 * - **3b** — Standard 3-byte (24-bit) addressing. Supports up to 16 MB.
 * - **4b** — Dedicated 4-byte address opcodes (0x13, 0x12, 0x21, 0xDC).
 *   Does not change global chip state. No 32 KB erase available.
 * - **4bMode** — Enter 4-Byte Address Mode (0xB7). Standard opcodes use
 *   4-byte addresses. All erase sizes available. Persists until exit or
 *   power cycle.
 * - **4bExReg** — Extended Address Register (0xC5/0xC8). Sets a bank
 *   register for the upper address bits. Standard opcodes address within
 *   the selected 16 MB bank. All erase sizes available.
 *
 * Users compose a whal_FlashDriver vtable from these functions to match
 * their specific flash part. Unused functions are stripped by the linker.
 *
 * The driver requires a SPI bus device and a GPIO pin for chip select.
 * Page size and total capacity are configurable to support a range of
 * SPI-NOR parts (W25Qxx, MX25L, AT25SF, S25FL, etc.).
 */

/*
 * @brief SPI-NOR device configuration.
 */
typedef struct whal_SpiNor_Cfg {
    whal_Spi *spiDev;           /* SPI bus device */
    whal_Spi_ComCfg *spiComCfg; /* SPI session config for StartCom */
    whal_Gpio *gpioDev;         /* GPIO device for chip select */
    size_t csPin;               /* GPIO pin index for chip select */
    whal_Timeout *timeout;
    size_t pageSz;              /* Page program size in bytes (typically 256) */
    size_t capacity;            /* Total flash capacity in bytes */
} whal_SpiNor_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_SPI_NOR_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
extern const whal_Flash whal_SpiNor_Dev;
#endif

/*
 * @brief Default driver vtable using 3-byte addressing with 4 KB erase.
 */
extern const whal_FlashDriver whal_SpiNor_Driver;

/*
 * @brief Initialize a SPI-NOR device in 3-byte address mode.
 *
 * Releases the device from Deep Power-Down, waits for any in-progress
 * operation, and reads the JEDEC ID to verify a device is present.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS    Device detected and ready.
 * @retval WHAL_EINVAL     Null pointer or invalid configuration.
 * @retval WHAL_EHARDWARE  No device detected (JEDEC ID 0x00 or 0xFF).
 * @retval WHAL_ETIMEOUT   Device did not become ready.
 */
whal_Error whal_SpiNor_Init(whal_Flash *flashDev);

/*
 * @brief Deinitialize a SPI-NOR device.
 *
 * Deasserts chip select. Does not send any commands to the device.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_SpiNor_Deinit(whal_Flash *flashDev);

/*
 * @brief Lock the entire device by setting all block protect bits.
 *
 * Writes SR1_BP0|BP1|BP2 to Status Register 1. The @p addr and @p len
 * parameters are ignored — the entire device is locked.
 *
 * @param flashDev Flash device instance.
 * @param addr     Ignored.
 * @param len      Ignored.
 *
 * @retval WHAL_SUCCESS    Block protect bits set.
 * @retval WHAL_EINVAL     Null pointer.
 * @retval WHAL_EHARDWARE  Write enable failed.
 */
whal_Error whal_SpiNor_Lock(whal_Flash *flashDev, size_t addr, size_t len);

/*
 * @brief Unlock the entire device by clearing all block protect bits.
 *
 * Writes 0x00 to Status Register 1. The @p addr and @p len parameters
 * are ignored — the entire device is unlocked.
 *
 * @param flashDev Flash device instance.
 * @param addr     Ignored.
 * @param len      Ignored.
 *
 * @retval WHAL_SUCCESS    Block protect bits cleared.
 * @retval WHAL_EINVAL     Null pointer.
 * @retval WHAL_EHARDWARE  Write enable failed.
 */
whal_Error whal_SpiNor_Unlock(whal_Flash *flashDev, size_t addr, size_t len);

/* -------------------------------------------------------------------- */
/*  Init / Deinit — 4-byte address mode                                 */
/* -------------------------------------------------------------------- */

/*
 * @brief Initialize a SPI-NOR device and enter 4-byte address mode.
 *
 * Performs standard initialization via whal_SpiNor_Init(), then sends the
 * Enter 4-Byte Address Mode command (0xB7). After this call, all 4bMode
 * variant functions can be used. The device remains in 4-byte mode until
 * power cycle or an explicit exit command.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS    Device initialized and 4-byte mode command sent.
 * @retval WHAL_EINVAL     Null pointer or invalid configuration.
 * @retval WHAL_EHARDWARE  No device detected.
 */
whal_Error whal_SpiNor4bMode_Init(whal_Flash *flashDev);

/* -------------------------------------------------------------------- */
/*  Read — 3-byte address                                               */
/* -------------------------------------------------------------------- */

/*
 * @brief Read data using the Read command (0x03) with 3-byte address.
 *
 * Standard read, limited to approximately 50 MHz clock speed.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start reading from.
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor3b_Read(whal_Flash *flashDev, size_t addr, void *data, size_t dataSz);

/*
 * @brief Read data using the Fast Read command (0x0B) with 3-byte address.
 *
 * Inserts one dummy byte after the address. Supports clock speeds up to
 * the device maximum (typically 104 MHz).
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start reading from.
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor3b_ReadFast(whal_Flash *flashDev, size_t addr, void *data, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Read — dedicated 4-byte address commands                            */
/* -------------------------------------------------------------------- */

/*
 * @brief Read data using the dedicated 4-byte Read command (0x13).
 *
 * Uses a device-specific opcode that carries a 4-byte address. Does not
 * require entering 4-byte address mode. Limited to approximately 50 MHz.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start reading from (up to 32 bits).
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor4b_Read(whal_Flash *flashDev, size_t addr, void *data, size_t dataSz);

/*
 * @brief Read data using the dedicated 4-byte Fast Read command (0x0C).
 *
 * Uses a device-specific opcode with 4-byte address and one dummy byte.
 * Supports full clock speed without entering 4-byte address mode.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start reading from (up to 32 bits).
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor4b_ReadFast(whal_Flash *flashDev, size_t addr, void *data, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Read — 4-byte address mode (after 0xB7)                             */
/* -------------------------------------------------------------------- */

/*
 * @brief Read data using Read (0x03) in 4-byte address mode.
 *
 * Requires prior call to whal_SpiNor4bMode_Init(). Uses the standard
 * opcode with a 4-byte address frame.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start reading from (up to 32 bits).
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor4bMode_Read(whal_Flash *flashDev, size_t addr, void *data, size_t dataSz);

/*
 * @brief Read data using Fast Read (0x0B) in 4-byte address mode.
 *
 * Requires prior call to whal_SpiNor4bMode_Init(). Uses the standard
 * opcode with a 4-byte address frame and one dummy byte.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start reading from (up to 32 bits).
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor4bMode_ReadFast(whal_Flash *flashDev, size_t addr, void *data, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Read — extended address register (bank select)                      */
/* -------------------------------------------------------------------- */

/*
 * @brief Read data using Read (0x03) with extended address register.
 *
 * Sets the extended address register (0xC5) to select the 16 MB bank
 * containing @p addr, then issues a standard 3-byte Read. Supports
 * addresses beyond 16 MB on parts that lack 4-byte address commands.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start reading from (up to 32 bits).
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor4bExReg_Read(whal_Flash *flashDev, size_t addr, void *data, size_t dataSz);

/*
 * @brief Read data using Fast Read (0x0B) with extended address register.
 *
 * Sets the extended address register then issues a standard 3-byte Fast
 * Read with one dummy byte.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start reading from (up to 32 bits).
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor4bExReg_ReadFast(whal_Flash *flashDev, size_t addr, void *data, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Write — 3-byte address                                              */
/* -------------------------------------------------------------------- */

/*
 * @brief Program data using Page Program (0x02) with 3-byte address.
 *
 * Automatically splits writes at page boundaries. Each page program is
 * preceded by a Write Enable and followed by a WIP poll.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start writing at.
 * @param data     Source buffer.
 * @param dataSz   Number of bytes to write.
 *
 * @retval WHAL_SUCCESS Write completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor3b_Write(whal_Flash *flashDev, size_t addr, const void *data, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Write — dedicated 4-byte address command                            */
/* -------------------------------------------------------------------- */

/*
 * @brief Program data using the dedicated 4-byte Page Program (0x12).
 *
 * Same behavior as whal_SpiNor3b_Write() but uses the dedicated 4-byte
 * opcode. Does not require entering 4-byte address mode.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start writing at (up to 32 bits).
 * @param data     Source buffer.
 * @param dataSz   Number of bytes to write.
 *
 * @retval WHAL_SUCCESS Write completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor4b_Write(whal_Flash *flashDev, size_t addr, const void *data, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Write — 4-byte address mode (after 0xB7)                            */
/* -------------------------------------------------------------------- */

/*
 * @brief Program data using Page Program (0x02) in 4-byte address mode.
 *
 * Requires prior call to whal_SpiNor4bMode_Init(). Uses the standard
 * opcode with a 4-byte address frame.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start writing at (up to 32 bits).
 * @param data     Source buffer.
 * @param dataSz   Number of bytes to write.
 *
 * @retval WHAL_SUCCESS Write completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor4bMode_Write(whal_Flash *flashDev, size_t addr, const void *data, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Write — extended address register                                   */
/* -------------------------------------------------------------------- */

/*
 * @brief Program data using Page Program (0x02) with extended address register.
 *
 * Sets the extended address register before each page program to select
 * the correct 16 MB bank. Re-sets the register on every page in case
 * the write crosses a bank boundary.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start writing at (up to 32 bits).
 * @param data     Source buffer.
 * @param dataSz   Number of bytes to write.
 *
 * @retval WHAL_SUCCESS Write completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, or out of bounds.
 */
whal_Error whal_SpiNor4bExReg_Write(whal_Flash *flashDev, size_t addr, const void *data, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Erase — 3-byte address                                              */
/* -------------------------------------------------------------------- */

/*
 * @brief Erase using 4 KB Sector Erase (0x20) with 3-byte address.
 *
 * Address must be 4 KB aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     4 KB-aligned byte address.
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor3b_Erase4k(whal_Flash *flashDev, size_t addr, size_t dataSz);

/*
 * @brief Erase using 32 KB Block Erase (0x52) with 3-byte address.
 *
 * Address must be 32 KB aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     32 KB-aligned byte address.
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor3b_Erase32k(whal_Flash *flashDev, size_t addr, size_t dataSz);

/*
 * @brief Erase using 64 KB Block Erase (0xD8) with 3-byte address.
 *
 * Address must be 64 KB aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     64 KB-aligned byte address.
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor3b_Erase64k(whal_Flash *flashDev, size_t addr, size_t dataSz);

/*
 * @brief Erase the entire device using Chip Erase (0xC7).
 *
 * The @p addr and @p dataSz parameters are ignored.
 *
 * @param flashDev Flash device instance.
 * @param addr     Ignored.
 * @param dataSz   Ignored.
 *
 * @retval WHAL_SUCCESS Chip erase completed.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_SpiNor_EraseChip(whal_Flash *flashDev, size_t addr, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Erase — dedicated 4-byte address commands                           */
/* -------------------------------------------------------------------- */

/*
 * @brief Erase using dedicated 4-byte Sector Erase (0x21).
 *
 * Address must be 4 KB aligned. No 32 KB variant exists in the dedicated
 * 4-byte command set.
 *
 * @param flashDev Flash device instance.
 * @param addr     4 KB-aligned byte address (up to 32 bits).
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor4b_Erase4k(whal_Flash *flashDev, size_t addr, size_t dataSz);

/*
 * @brief Erase using dedicated 4-byte Block Erase (0xDC).
 *
 * Address must be 64 KB aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     64 KB-aligned byte address (up to 32 bits).
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor4b_Erase64k(whal_Flash *flashDev, size_t addr, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Erase — 4-byte address mode (after 0xB7)                            */
/* -------------------------------------------------------------------- */

/*
 * @brief Erase using Sector Erase (0x20) in 4-byte address mode.
 *
 * Requires prior call to whal_SpiNor4bMode_Init(). Address must be
 * 4 KB aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     4 KB-aligned byte address (up to 32 bits).
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor4bMode_Erase4k(whal_Flash *flashDev, size_t addr, size_t dataSz);

/*
 * @brief Erase using Block Erase (0x52) in 4-byte address mode.
 *
 * Requires prior call to whal_SpiNor4bMode_Init(). Address must be
 * 32 KB aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     32 KB-aligned byte address (up to 32 bits).
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor4bMode_Erase32k(whal_Flash *flashDev, size_t addr, size_t dataSz);

/*
 * @brief Erase using Block Erase (0xD8) in 4-byte address mode.
 *
 * Requires prior call to whal_SpiNor4bMode_Init(). Address must be
 * 64 KB aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     64 KB-aligned byte address (up to 32 bits).
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor4bMode_Erase64k(whal_Flash *flashDev, size_t addr, size_t dataSz);

/* -------------------------------------------------------------------- */
/*  Erase — extended address register                                   */
/* -------------------------------------------------------------------- */

/*
 * @brief Erase using Sector Erase (0x20) with extended address register.
 *
 * Sets the extended address register before each erase to select the
 * correct 16 MB bank. Address must be 4 KB aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     4 KB-aligned byte address (up to 32 bits).
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor4bExReg_Erase4k(whal_Flash *flashDev, size_t addr, size_t dataSz);

/*
 * @brief Erase using Block Erase (0x52) with extended address register.
 *
 * Sets the extended address register before each erase. Address must be
 * 32 KB aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     32 KB-aligned byte address (up to 32 bits).
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor4bExReg_Erase32k(whal_Flash *flashDev, size_t addr, size_t dataSz);

/*
 * @brief Erase using Block Erase (0xD8) with extended address register.
 *
 * Sets the extended address register before each erase. Address must be
 * 64 KB aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     64 KB-aligned byte address (up to 32 bits).
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer, zero size, out of bounds, or unaligned.
 */
whal_Error whal_SpiNor4bExReg_Erase64k(whal_Flash *flashDev, size_t addr, size_t dataSz);

#endif /* WHAL_SPI_NOR_H */
