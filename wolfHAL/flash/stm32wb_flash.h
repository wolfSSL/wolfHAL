/* stm32wb_flash.h
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

#ifndef WHAL_STM32WB_FLASH_H
#define WHAL_STM32WB_FLASH_H

#include <wolfHAL/flash/flash.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32wb_flash.h
 * @brief STM32WB flash driver configuration and helpers.
 *
 * The STM32WB embedded flash provides:
 * - Up to 1 MB of flash memory organized in 4 KB pages
 * - Double-word (64-bit) programming
 * - Page erase and mass erase operations
 * - Read-while-write capability on different banks
 * - Configurable wait states based on CPU frequency
 *
 * Flash must be unlocked before write/erase operations and locked
 * afterward for protection. Wait states must be configured appropriately
 * for the system clock frequency.
 */

/*
 * @brief Flash device configuration.
 */
typedef struct whal_Stm32wb_Flash_Cfg {
    size_t startAddr;     /* Flash base address (typically 0x08000000) */
    size_t size;          /* Flash size in bytes */
    whal_Timeout *timeout;
} whal_Stm32wb_Flash_Cfg;

/*
 * @brief Flash access latency (wait states).
 *
 * The number of wait states must be configured based on the CPU frequency
 * and supply voltage. Insufficient wait states will cause flash read errors.
 *
 * Typical settings at VOS1 (1.2V):
 *   - 0 WS: up to 18 MHz
 *   - 1 WS: up to 36 MHz
 *   - 2 WS: up to 54 MHz
 *   - 3 WS: up to 64 MHz
 */
typedef enum whal_Stm32wb_Flash_Latency {
    WHAL_STM32WB_FLASH_LATENCY_0, /* 0 wait states */
    WHAL_STM32WB_FLASH_LATENCY_1, /* 1 wait state */
    WHAL_STM32WB_FLASH_LATENCY_2, /* 2 wait states */
    WHAL_STM32WB_FLASH_LATENCY_3, /* 3 wait states */
} whal_Stm32wb_Flash_Latency;

#ifndef WHAL_CFG_STM32WB_FLASH_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32 flash.
 */
extern const whal_FlashDriver whal_Stm32wb_Flash_Driver;

/*
 * @brief Platform-owned flash device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32WB_FLASH_DEV initializer in wolfHAL_board.h. Other TUs
 * may take its address (e.g. via BOARD_FLASH_DEV) but should not mutate
 * it.
 */
extern const whal_Flash whal_Stm32wb_Flash_Dev;

/*
 * @brief Initialize the STM32 flash interface.
 *
 * @param flashDev Flash device instance to initialize.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Flash_Init(whal_Flash *flashDev);
/*
 * @brief Deinitialize the STM32 flash interface.
 *
 * @param flashDev Flash device instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Flash_Deinit(whal_Flash *flashDev);
/*
 * @brief Lock a flash range.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to lock.
 * @param len      Number of bytes to lock.
 *
 * @retval WHAL_SUCCESS Lock applied.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len);
/*
 * @brief Unlock a flash range.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to unlock.
 * @param len      Number of bytes to unlock.
 *
 * @retval WHAL_SUCCESS Unlock applied.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len);
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
whal_Error whal_Stm32wb_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                             size_t dataSz);
/*
 * @brief Write a block of data to flash.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to program.
 * @param data     Buffer to program.
 * @param dataSz   Number of bytes to program.
 *
 * @retval WHAL_SUCCESS Program completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Flash_Write(whal_Flash *flashDev, size_t addr, const void *data,
                              size_t dataSz);
/*
 * @brief Erase a flash range.
 *
 * @param flashDev Flash device instance.
 * @param addr     Flash address to start erasing.
 * @param dataSz   Number of bytes to erase.
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Flash_Erase(whal_Flash *flashDev, size_t addr, size_t dataSz);
#endif /* !WHAL_CFG_STM32WB_FLASH_DIRECT_API_MAPPING */
/*
 * @brief Update flash latency wait states.
 *
 * @param flashDev Flash device instance.
 * @param latency  Latency setting to apply.
 *
 * @retval WHAL_SUCCESS Latency updated.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32wb_Flash_Ext_SetLatency(whal_Flash *flashDev, enum whal_Stm32wb_Flash_Latency latency);

#endif /* WHAL_STM32WB_FLASH_H */
