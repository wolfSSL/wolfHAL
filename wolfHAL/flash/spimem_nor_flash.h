/* spimem_nor_flash.h
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

#ifndef WHAL_SPIMEM_NOR_FLASH_H
#define WHAL_SPIMEM_NOR_FLASH_H

#include <stddef.h>
#include <stdint.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/spimem/spimem.h>
#include <wolfHAL/timeout.h>

/*
 * @file spimem_nor_flash.h
 * @brief NOR flash driver over a SPI memory controller.
 *
 * 1-bit (1-1-1) commands, one whal_SpiMem_Exec() each. Never sends protect,
 * lock, OTP or nonvolatile config commands.
 */

/* The flash has a Micron flag status register (70h). Program and erase
 * errors are checked in it and cleared with 50h. */
#define WHAL_SPIMEM_NOR_FLAG_FSR (1UL << 0)

/* If the ID read fails, Init sends 66h, 99h, waits and reads the ID again.
 * The reset loads the nonvolatile config (1-bit by default). */
#define WHAL_SPIMEM_NOR_FLAG_RESET (1UL << 1)

/* With FLAG_RESET: 66h, 99h in octal DDR first, for a flash left in octal
 * DDR. If the controller cannot, the 1-bit reset still runs. */
#define WHAL_SPIMEM_NOR_FLAG_RESET_OCTAL_DTR (1UL << 2)

/* With FLAG_RESET: also write FFh (1-bit) to Micron volatile config 00h
 * (81h); address size from FSR (70h) bit 0. */
#define WHAL_SPIMEM_NOR_FLAG_MICRON_VCFG_1BIT (1UL << 3)

/*
 * @brief NOR flash configuration.
 */
typedef struct whal_SpiMemNor_Flash_Cfg {
    whal_SpiMem *spiMemDev;  /* SPI memory controller */
    size_t capacity;         /* flash size in bytes */
    size_t pageSz;           /* program page in bytes, a power of two */
    size_t eraseSz;          /* erase block in bytes, a power of two */
    uint8_t addrBytes;       /* 3 or 4, for read, program, erase */
    uint8_t readOp;          /* read opcode */
    uint8_t readDummy;       /* dummy cycles of the read opcode */
    uint8_t progOp;          /* page program opcode */
    uint8_t eraseOp;         /* erase opcode for one eraseSz block */
    uint32_t flags;          /* WHAL_SPIMEM_NOR_FLAG_* */
    uint32_t id;             /* expected JEDEC ID, byte 0 in bits 7:0;
                              * 0 = any flash */
    whal_Timeout *timeout;   /* optional timeout for each status wait */
} whal_SpiMemNor_Flash_Cfg;

/*
 * @brief Read the 3-byte JEDEC ID (9Fh) in 1-bit mode.
 *
 * @param flashDev Flash device instance.
 * @param id       Receives manufacturer, type and capacity.
 *
 * @retval WHAL_SUCCESS ID read.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_SpiMemNor_Flash_ReadId(whal_Flash *flashDev, uint8_t id[3]);

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_SPIMEM_NOR_FLASH_DEV initializer in wolfHAL_board.h.
 */
#ifdef WHAL_CFG_SPIMEM_NOR_FLASH_SINGLE_INSTANCE
extern const whal_Flash whal_SpiMemNor_Flash_Dev;
#endif

#ifndef WHAL_CFG_SPIMEM_NOR_FLASH_DIRECT_API_MAPPING
/*
 * @brief Driver instance for NOR flash over a SPI memory controller.
 */
extern const whal_FlashDriver whal_SpiMemNor_Flash_Driver;

/*
 * @brief Check the configuration and look for the flash.
 *
 * Reads the JEDEC ID (not 00h/FFh, equal to cfg->id if set); on failure
 * with FLAG_RESET, resets and retries. With FLAG_FSR, clears FSR errors.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS   Flash found.
 * @retval WHAL_EINVAL    Null pointer or invalid configuration.
 * @retval WHAL_EHARDWARE No flash found, or not the expected ID.
 * @retval WHAL_ETIMEOUT  The flash stayed busy after the reset.
 */
whal_Error whal_SpiMemNor_Flash_Init(whal_Flash *flashDev);

/*
 * @brief Deinitialize the driver. Sends no commands.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_SpiMemNor_Flash_Deinit(whal_Flash *flashDev);

/*
 * @brief Lock the flash. Does nothing.
 *
 * No protect commands are sent: the flash stays writable. The caller
 * guards the regions it must not change.
 *
 * @param flashDev Flash device instance.
 * @param addr     Ignored.
 * @param len      Ignored.
 *
 * @retval WHAL_SUCCESS Always, for a valid device.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_SpiMemNor_Flash_Lock(whal_Flash *flashDev, size_t addr,
                                     size_t len);

/*
 * @brief Unlock the flash. Does nothing.
 *
 * No protect commands are sent, so the block protect bits are not changed.
 *
 * @param flashDev Flash device instance.
 * @param addr     Ignored.
 * @param len      Ignored.
 *
 * @retval WHAL_SUCCESS Always, for a valid device.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_SpiMemNor_Flash_Unlock(whal_Flash *flashDev, size_t addr,
                                       size_t len);

/*
 * @brief Read data with the configured read opcode.
 *
 * Sends one read operation. The controller may split it.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to read from.
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read. 0 does nothing.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null pointer or out of bounds.
 */
whal_Error whal_SpiMemNor_Flash_Read(whal_Flash *flashDev, size_t addr,
                                     void *data, size_t dataSz);

/*
 * @brief Program data with the configured program opcode.
 *
 * Splits at pages and the controller's max size. Each piece: WREN, program,
 * wait, FSR check (FLAG_FSR). The region must be erased first.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address to start writing at.
 * @param data     Source buffer.
 * @param dataSz   Number of bytes to write. 0 does nothing.
 *
 * @retval WHAL_SUCCESS   Write completed.
 * @retval WHAL_EINVAL    Null pointer, out of bounds, or the controller
 *                        reports a largest program size of 0.
 * @retval WHAL_ETIMEOUT  The flash did not set the write enable latch or
 *                        did not finish in time.
 * @retval WHAL_EHARDWARE The flag status register reported an error.
 */
whal_Error whal_SpiMemNor_Flash_Write(whal_Flash *flashDev, size_t addr,
                                      const void *data, size_t dataSz);

/*
 * @brief Erase blocks with the configured erase opcode.
 *
 * One eraseSz block per command: WREN, erase, wait, FSR check (FLAG_FSR).
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address, a multiple of eraseSz.
 * @param dataSz   Number of bytes, a multiple of eraseSz. 0 does nothing.
 *
 * @retval WHAL_SUCCESS   Erase completed.
 * @retval WHAL_EINVAL    Null pointer, out of bounds, or not aligned.
 * @retval WHAL_ETIMEOUT  The flash did not set the write enable latch or
 *                        did not finish in time.
 * @retval WHAL_EHARDWARE The flag status register reported an error.
 */
whal_Error whal_SpiMemNor_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                      size_t dataSz);
#endif /* !WHAL_CFG_SPIMEM_NOR_FLASH_DIRECT_API_MAPPING */

#endif /* WHAL_SPIMEM_NOR_FLASH_H */
