/* stm32h5_flash.h
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

#ifndef WHAL_STM32H5_FLASH_H
#define WHAL_STM32H5_FLASH_H

#include <wolfHAL/flash/flash.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32h5_flash.h
 * @brief STM32H5 flash driver configuration.
 *
 * The STM32H5 embedded flash provides:
 * - Up to 2 MB organized in dual banks of 8 KB sectors
 * - 128-bit (flash-word) programming with write buffer
 * - Sector erase and bank/mass erase operations
 * - Non-secure and secure register variants (this driver uses non-secure)
 */

/*
 * @brief Flash device configuration.
 */
typedef struct whal_Stm32h5_Flash_Cfg {
    size_t startAddr;     /* Flash base address (typically 0x08000000) */
    size_t size;          /* Flash size in bytes */
    whal_Timeout *timeout;
} whal_Stm32h5_Flash_Cfg;

#ifndef WHAL_CFG_STM32H5_FLASH_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32H5 flash.
 */
extern const whal_FlashDriver whal_Stm32h5_Flash_Driver;
/*
 * @brief Platform-owned device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32H5_FLASH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Flash whal_Stm32h5_Flash_Dev;

/*
 * @brief Initialize the STM32H5 flash interface.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Flash_Init(whal_Flash *flashDev);

/*
 * @brief Deinitialize the STM32H5 flash interface.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Flash_Deinit(whal_Flash *flashDev);

/*
 * @brief Lock the flash control register.
 *
 * @param flashDev Flash device instance.
 * @param addr     Unused.
 * @param len      Unused.
 *
 * @retval WHAL_SUCCESS Lock applied.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Flash_Lock(whal_Flash *flashDev, size_t addr,
                                   size_t len);

/*
 * @brief Unlock the flash control register.
 *
 * @param flashDev Flash device instance.
 * @param addr     Unused.
 * @param len      Unused.
 *
 * @retval WHAL_SUCCESS Unlock applied.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Flash_Unlock(whal_Flash *flashDev, size_t addr,
                                     size_t len);

/*
 * @brief Read data from flash into a buffer.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to read.
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 */
whal_Error whal_Stm32h5_Flash_Read(whal_Flash *flashDev, size_t addr,
                                   void *data, size_t dataSz);

/*
 * @brief Write data to flash.
 *
 * Data is programmed in 128-bit (16 byte) flash-word chunks.
 * Address and size must be 16-byte aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to program (16-byte aligned).
 * @param data     Buffer to program.
 * @param dataSz   Number of bytes to program (multiple of 16).
 *
 * @retval WHAL_SUCCESS   Program completed.
 * @retval WHAL_EINVAL    Invalid arguments or alignment.
 * @retval WHAL_EHARDWARE Flash error during programming.
 */
whal_Error whal_Stm32h5_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz);

/*
 * @brief Erase flash sectors covering the given range.
 *
 * Erases 8 KB sectors. Address must fall within configured flash bounds.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to start erasing.
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS   Erase completed.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_EHARDWARE Flash error during erase.
 */
whal_Error whal_Stm32h5_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                    size_t dataSz);
#endif /* !WHAL_CFG_STM32H5_FLASH_DIRECT_API_MAPPING */

/*
 * @brief Set flash access latency (wait states) and write high-frequency mode.
 *
 * Must be called before increasing the system clock to avoid flash access
 * faults. The value is written directly to FLASH_ACR[7:0]:
 *   bits [3:0] = LATENCY (wait states, 0-15)
 *   bits [5:4] = WRHIGHFREQ (0-2, based on frequency range)
 *
 * @param flashDev Flash device instance.
 * @param latency  Combined LATENCY | (WRHIGHFREQ << 4) value.
 *
 * @retval WHAL_SUCCESS Latency updated.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Flash_Ext_SetLatency(whal_Flash *flashDev,
                                             uint8_t latency);

#endif /* WHAL_STM32H5_FLASH_H */
