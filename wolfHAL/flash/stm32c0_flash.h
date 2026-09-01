/* stm32c0_flash.h
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

#ifndef WHAL_STM32C0_FLASH_H
#define WHAL_STM32C0_FLASH_H

#include <wolfHAL/flash/flash.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32c0_flash.h
 * @brief STM32C0 flash driver configuration and helpers.
 *
 * The STM32C0 embedded flash provides:
 * - Up to 32 KB of flash memory organized in 2 KB pages
 * - Double-word (64-bit) programming
 * - Page erase and mass erase operations
 * - Configurable wait states based on CPU frequency
 *
 * Flash must be unlocked before write/erase operations and locked
 * afterward for protection. Wait states must be configured appropriately
 * for the system clock frequency.
 */

/*
 * @brief Flash device configuration.
 */
typedef struct whal_Stm32c0_Flash_Cfg {
    size_t startAddr;     /* Flash base address (typically 0x08000000) */
    size_t size;          /* Flash size in bytes */
    whal_Timeout *timeout;
} whal_Stm32c0_Flash_Cfg;

/*
 * @brief Flash access latency (wait states).
 *
 * The number of wait states must be configured based on the CPU frequency.
 *   - 0 WS: up to 24 MHz
 *   - 1 WS: up to 48 MHz
 */
typedef enum whal_Stm32c0_Flash_Latency {
    WHAL_STM32C0_FLASH_LATENCY_0, /* 0 wait states */
    WHAL_STM32C0_FLASH_LATENCY_1, /* 1 wait state */
} whal_Stm32c0_Flash_Latency;

#ifndef WHAL_CFG_STM32C0_FLASH_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32C0 flash.
 */
extern const whal_FlashDriver whal_Stm32c0_Flash_Driver;

/*
 * @brief Platform-owned flash device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32C0_FLASH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Flash whal_Stm32c0_Flash_Dev;

/*
 * @brief Initialize the STM32C0 flash interface.
 *
 * @param flashDev Flash device instance to initialize.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32c0_Flash_Init(whal_Flash *flashDev);

/*
 * @brief Deinitialize the STM32C0 flash interface.
 *
 * @param flashDev Flash device instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32c0_Flash_Deinit(whal_Flash *flashDev);

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
whal_Error whal_Stm32c0_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len);

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
whal_Error whal_Stm32c0_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len);

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
whal_Error whal_Stm32c0_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                             size_t dataSz);

/*
 * @brief Write a block of data to flash.
 *
 * Data is programmed in 64-bit (8 byte) double-word chunks.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to program.
 * @param data     Buffer to program.
 * @param dataSz   Number of bytes to program.
 *
 * @retval WHAL_SUCCESS Program completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32c0_Flash_Write(whal_Flash *flashDev, size_t addr, const void *data,
                              size_t dataSz);

/*
 * @brief Erase flash pages covering the given range.
 *
 * Erases 2 KB pages. Address must fall within configured flash bounds.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to start erasing.
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32c0_Flash_Erase(whal_Flash *flashDev, size_t addr, size_t dataSz);
#endif /* !WHAL_CFG_STM32C0_FLASH_DIRECT_API_MAPPING */

/*
 * @brief Update flash latency wait states.
 *
 * @param flashDev Flash device instance.
 * @param latency  Latency setting to apply.
 *
 * @retval WHAL_SUCCESS Latency updated.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32c0_Flash_Ext_SetLatency(whal_Flash *flashDev, enum whal_Stm32c0_Flash_Latency latency);

#endif /* WHAL_STM32C0_FLASH_H */
