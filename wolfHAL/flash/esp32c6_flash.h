/* esp32c6_flash.h
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

#ifndef WHAL_ESP32C6_FLASH_H
#define WHAL_ESP32C6_FLASH_H

#include <stddef.h>
#include <stdint.h>
#include <wolfHAL/flash/flash.h>

/*
 * @file esp32c6_flash.h
 * @brief ESP32-C6 SPI flash driver.
 *
 * The program runs in place from SPI NOR flash mapped through the cache at
 * 0x4200_0000. This driver erases, programs and reads that flash with the
 * SPI1 memory-SPI controller. Addresses are given in the XIP window
 * (startAddr maps to flash offset 0), so the region the running image
 * occupies can be told apart from free flash.
 *
 * While a command runs the flash cannot serve instruction fetches, so each
 * command executes from SRAM (.ramfunc, copied in at startup) with
 * interrupts disabled until the flash reports idle. The busy-wait has no
 * timeout because the tick stops while interrupts are off. Reads go through
 * SPI1 rather than the cache; XIP reads of a region this driver modified may
 * return stale cached data.
 */

/*
 * @brief Flash device configuration.
 */
typedef struct {
    size_t startAddr; /* XIP address of flash offset 0 (0x42000000) */
    size_t size;      /* Flash size in bytes */
} whal_Esp32c6_Flash_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_ESP32C6_FLASH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Flash whal_Esp32c6_Flash_Dev;

#ifndef WHAL_CFG_ESP32C6_FLASH_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the ESP32-C6 SPI flash.
 */
extern const whal_FlashDriver whal_Esp32c6_Flash_Driver;

/*
 * @brief Initialize the flash driver. SPI1 is already configured by the ROM.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_Flash_Init(whal_Flash *flashDev);

/*
 * @brief Deinitialize the flash driver.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_Flash_Deinit(whal_Flash *flashDev);

/*
 * @brief No-op; the flash block protection bits are left cleared.
 *
 * @param flashDev Flash device instance.
 * @param addr     Start address (unused).
 * @param len      Length in bytes (unused).
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len);

/*
 * @brief No-op; the flash block protection bits are left cleared.
 *
 * @param flashDev Flash device instance.
 * @param addr     Start address (unused).
 * @param len      Length in bytes (unused).
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len);

/*
 * @brief Read from flash through SPI1.
 *
 * @param flashDev Flash device instance.
 * @param addr     XIP address to read from.
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Data read.
 * @retval WHAL_EINVAL  NULL data or range outside the flash.
 */
whal_Error whal_Esp32c6_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                                   size_t dataSz);

/*
 * @brief Program flash. The target range must be erased; any alignment and
 * length is accepted.
 *
 * @param flashDev Flash device instance.
 * @param addr     XIP address to program.
 * @param data     Source buffer.
 * @param dataSz   Number of bytes to program.
 *
 * @retval WHAL_SUCCESS Data programmed.
 * @retval WHAL_EINVAL  NULL data or range outside the flash.
 */
whal_Error whal_Esp32c6_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz);

/*
 * @brief Erase every 4 KB sector overlapping [addr, addr + dataSz).
 *
 * @param flashDev Flash device instance.
 * @param addr     XIP address of the range start.
 * @param dataSz   Length of the range in bytes.
 *
 * @retval WHAL_SUCCESS Sectors erased.
 * @retval WHAL_EINVAL  Range outside the flash.
 */
whal_Error whal_Esp32c6_Flash_Erase(whal_Flash *flashDev, size_t addr, size_t dataSz);
#endif /* !WHAL_CFG_ESP32C6_FLASH_DIRECT_API_MAPPING */

#endif /* WHAL_ESP32C6_FLASH_H */
