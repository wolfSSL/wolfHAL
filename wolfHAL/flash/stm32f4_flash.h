/* stm32f4_flash.h
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

#ifndef WHAL_STM32F4_FLASH_H
#define WHAL_STM32F4_FLASH_H

#include <wolfHAL/flash/flash.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32f4_flash.h
 * @brief STM32F4 flash driver configuration.
 *
 * The STM32F4 embedded flash provides:
 * - Up to 512 KB organized in variable-size sectors
 * - Byte, half-word, word, and double-word programming
 * - Sector erase and mass erase operations
 * - Configurable wait states based on CPU frequency and voltage
 *
 * Sector layout for STM32F411xE (512 KB):
 *   Sectors 0-3: 16 KB each
 *   Sector 4: 64 KB
 *   Sectors 5-7: 128 KB each
 *
 * Flash must be unlocked before write/erase operations and locked
 * afterward for protection.
 */

/*
 * @brief Flash sector descriptor.
 *
 * Describes a flash sector's base address and size for the sector-based
 * erase mechanism used by the STM32F4 flash controller.
 */
typedef struct whal_Stm32f4_Flash_Sector {
    size_t addr; /* Sector base address */
    size_t size; /* Sector size in bytes */
} whal_Stm32f4_Flash_Sector;

/*
 * @brief Flash device configuration.
 */
typedef struct whal_Stm32f4_Flash_Cfg {
    size_t startAddr;                          /* Flash base address (0x08000000) */
    size_t size;                               /* Flash size in bytes */
    const whal_Stm32f4_Flash_Sector *sectors;   /* Sector descriptor array */
    size_t sectorCount;                        /* Number of sectors */
    whal_Timeout *timeout;
} whal_Stm32f4_Flash_Cfg;

/*
 * @brief Flash access latency (wait states).
 *
 * At 2.7-3.6V supply:
 *   0 WS: up to 30 MHz
 *   1 WS: up to 64 MHz
 *   2 WS: up to 90 MHz
 *   3 WS: up to 100 MHz
 */
typedef enum whal_Stm32f4_Flash_Latency {
    WHAL_STM32F4_FLASH_LATENCY_0, /* 0 wait states */
    WHAL_STM32F4_FLASH_LATENCY_1, /* 1 wait state */
    WHAL_STM32F4_FLASH_LATENCY_2, /* 2 wait states */
    WHAL_STM32F4_FLASH_LATENCY_3, /* 3 wait states */
} whal_Stm32f4_Flash_Latency;

#ifndef WHAL_CFG_STM32F4_FLASH_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32F4 flash.
 */
extern const whal_FlashDriver whal_Stm32f4_Flash_Driver;
/*
 * @brief Platform-owned device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32F4_FLASH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Flash whal_Stm32f4_Flash_Dev;

/*
 * @brief Initialize the STM32F4 flash interface.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32f4_Flash_Init(whal_Flash *flashDev);

/*
 * @brief Deinitialize the STM32F4 flash interface.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32f4_Flash_Deinit(whal_Flash *flashDev);

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
whal_Error whal_Stm32f4_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len);

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
whal_Error whal_Stm32f4_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len);

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
whal_Error whal_Stm32f4_Flash_Read(whal_Flash *flashDev, size_t addr,
                                   void *data, size_t dataSz);

/*
 * @brief Write data to flash.
 *
 * Data is programmed in 32-bit word chunks. Address and size must be
 * 4-byte aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to program (4-byte aligned).
 * @param data     Buffer to program.
 * @param dataSz   Number of bytes to program (multiple of 4).
 *
 * @retval WHAL_SUCCESS   Program completed.
 * @retval WHAL_EINVAL    Invalid arguments or alignment.
 * @retval WHAL_EHARDWARE Flash error during programming.
 */
whal_Error whal_Stm32f4_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz);

/*
 * @brief Erase flash sectors covering the given range.
 *
 * Erases variable-size sectors. Address must fall within configured
 * flash bounds.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to start erasing.
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS   Erase completed.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_EHARDWARE Flash error during erase.
 */
whal_Error whal_Stm32f4_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                    size_t dataSz);
#endif /* !WHAL_CFG_STM32F4_FLASH_DIRECT_API_MAPPING */

/*
 * @brief Update flash latency wait states.
 *
 * @param flashDev Flash device instance.
 * @param latency  Latency setting to apply.
 *
 * @retval WHAL_SUCCESS Latency updated.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32f4_Flash_Ext_SetLatency(whal_Flash *flashDev,
                                             enum whal_Stm32f4_Flash_Latency latency);

#endif /* WHAL_STM32F4_FLASH_H */
