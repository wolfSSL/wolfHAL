/* pic32cz_flash.h
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

#ifndef WHAL_PIC32CZ_FLASH_H
#define WHAL_PIC32CZ_FLASH_H

#include <wolfHAL/flash/flash.h>
#include <wolfHAL/timeout.h>

/*
 * @file pic32cz_flash.h
 * @brief PIC32CZ FCW (Flash Controller Write) driver configuration.
 *
 * The PIC32CZ flash is organized as:
 *   Page (erase unit)       = 4096 bytes
 *   Row (row-write unit)    = 1024 bytes
 *   Quad double word        = 32 bytes (8 x uint32)
 *   Single double word      = 8 bytes (2 x uint32)
 *
 * Write operations require double-word (8-byte) aligned addresses and sizes.
 * Erase operations erase full 4 KB pages.
 * Each write/erase operation requires an unlock key written to FCW_KEY.
 */

#define WHAL_PIC32CZ_FLASH_PAGE_SIZE    4096
#define WHAL_PIC32CZ_FLASH_DWORD_SIZE   8
#define WHAL_PIC32CZ_FLASH_QDWORD_SIZE  32

/*
 * @brief Flash device configuration.
 */
typedef struct whal_Pic32cz_Flash_Cfg {
    size_t startAddr;
    size_t size;
    whal_Timeout *timeout;
} whal_Pic32cz_Flash_Cfg;

#ifndef WHAL_CFG_PIC32CZ_FLASH_DIRECT_API_MAPPING
/*
 * @brief Driver instance for PIC32CZ flash.
 */
extern const whal_FlashDriver whal_Pic32cz_Flash_Driver;
/*
 * @brief Platform-owned device singleton. Defined in the driver TU
 * from the WHAL_CFG_PIC32CZ_FLASH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Flash whal_Pic32cz_Flash_Dev;

/*
 * @brief Initialize the PIC32CZ flash interface.
 *
 * @param flashDev Flash device instance to initialize.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Flash_Init(whal_Flash *flashDev);
/*
 * @brief Deinitialize the PIC32CZ flash interface.
 *
 * @param flashDev Flash device instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Flash_Deinit(whal_Flash *flashDev);
/*
 * @brief Lock a flash range (stub, not yet implemented).
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to lock.
 * @param len      Number of bytes to lock.
 *
 * @retval WHAL_SUCCESS Lock applied.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len);
/*
 * @brief Unlock a flash range (stub, not yet implemented).
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to unlock.
 * @param len      Number of bytes to unlock.
 *
 * @retval WHAL_SUCCESS Unlock applied.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len);
/*
 * @brief Read data from flash into a buffer.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to read.
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                             size_t dataSz);
/*
 * @brief Write a block of data to flash.
 *
 * Address and size must be double-word (8-byte) aligned. Uses quad double
 * word writes (32 bytes) where alignment permits, falling back to single
 * double word writes (8 bytes) for the remainder.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to program (8-byte aligned).
 * @param data     Buffer to program (8-byte aligned).
 * @param dataSz   Number of bytes to program (multiple of 8).
 *
 * @retval WHAL_SUCCESS Program completed.
 * @retval WHAL_EINVAL  Invalid arguments or alignment.
 */
whal_Error whal_Pic32cz_Flash_Write(whal_Flash *flashDev, size_t addr, const void *data,
                              size_t dataSz);
/*
 * @brief Erase flash pages covering the given range.
 *
 * Erases all 4 KB pages that overlap the address range [addr, addr+dataSz).
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to start erasing.
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Pic32cz_Flash_Erase(whal_Flash *flashDev, size_t addr, size_t dataSz);
#endif /* !WHAL_CFG_PIC32CZ_FLASH_DIRECT_API_MAPPING */

#endif /* WHAL_PIC32CZ_FLASH_H */
