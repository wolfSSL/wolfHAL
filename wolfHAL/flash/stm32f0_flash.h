/* stm32f0_flash.h
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

#ifndef WHAL_STM32F0_FLASH_H
#define WHAL_STM32F0_FLASH_H

#include <wolfHAL/flash/flash.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32f0_flash.h
 * @brief STM32F0 flash driver configuration.
 *
 * The STM32F0 embedded flash provides:
 * - Up to 256 KB organized in 2 KB pages
 * - Half-word (16-bit) programming
 * - Page erase via FLASH_AR address register
 * - Configurable wait states based on CPU frequency
 */

/*
 * @brief STM32F0 flash driver configuration.
 */
typedef struct whal_Stm32f0_Flash_Cfg {
    size_t startAddr;            /* Flash region start address */
    size_t size;                 /* Flash region size in bytes */
    whal_Timeout *timeout;       /* Optional timeout for poll loops */
} whal_Stm32f0_Flash_Cfg;

/*
 * @brief Flash latency (FLASH_ACR.LATENCY) — number of CPU wait states.
 */
typedef enum whal_Stm32f0_Flash_Latency {
    WHAL_STM32F0_FLASH_LATENCY_0, /* 0 wait states (HCLK <= 24 MHz) */
    WHAL_STM32F0_FLASH_LATENCY_1, /* 1 wait state  (HCLK <= 48 MHz) */
} whal_Stm32f0_Flash_Latency;

#ifndef WHAL_CFG_STM32F0_FLASH_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the STM32F0 embedded flash controller.
 */
extern const whal_FlashDriver whal_Stm32f0_Flash_Driver;
/*
 * @brief Platform-owned device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32F0_FLASH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Flash whal_Stm32f0_Flash_Dev;

/*
 * @brief Initialize the STM32F0 flash driver. Mainly validates cfg and is
 *        a no-op against the hardware (flash boots ready).
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Driver is ready.
 * @retval WHAL_EINVAL  Null pointer or missing cfg.
 */
whal_Error whal_Stm32f0_Flash_Init(whal_Flash *flashDev);

/*
 * @brief Deinitialize the STM32F0 flash driver (re-locks if unlocked).
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Driver is deinitialized.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Stm32f0_Flash_Deinit(whal_Flash *flashDev);

/*
 * @brief Re-lock the STM32F0 flash for writes (sets FLASH_CR.LOCK).
 *
 * @param flashDev Flash device instance.
 * @param addr     Range start (ignored — flash is globally locked).
 * @param len      Range length (ignored).
 *
 * @retval WHAL_SUCCESS Flash is locked.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Stm32f0_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len);

/*
 * @brief Unlock the STM32F0 flash for writes via the KEYR sequence.
 *
 * @param flashDev Flash device instance.
 * @param addr     Range start (ignored — flash is globally unlocked).
 * @param len      Range length (ignored).
 *
 * @retval WHAL_SUCCESS Flash is unlocked.
 * @retval WHAL_EINVAL  Null pointer.
 * @retval WHAL_EHARDWARE Key sequence rejected (already unlocked or option-byte locked).
 */
whal_Error whal_Stm32f0_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len);

/*
 * @brief Read `dataSz` bytes from flash at `addr` into `data`.
 *
 * @param flashDev Flash device instance.
 * @param addr     Source address within the flash region.
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null pointer or address out of region.
 */
whal_Error whal_Stm32f0_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                                   size_t dataSz);

/*
 * @brief Program `dataSz` bytes from `data` to flash at `addr`. Address and
 *        size must be 16-bit (half-word) aligned.
 *
 * @param flashDev Flash device instance.
 * @param addr     Destination address within the flash region.
 * @param data     Source buffer.
 * @param dataSz   Number of bytes to write.
 *
 * @retval WHAL_SUCCESS Write completed.
 * @retval WHAL_EINVAL  Null pointer, misaligned address/size, or address out of region.
 * @retval WHAL_ETIMEOUT BSY did not clear within the configured timeout.
 * @retval WHAL_EHARDWARE Programming error reported in FLASH_SR.
 */
whal_Error whal_Stm32f0_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz);

/*
 * @brief Erase one or more 2 KB pages covering [addr, addr+dataSz).
 *
 * @param flashDev Flash device instance.
 * @param addr     First page address.
 * @param dataSz   Number of bytes to erase (rounded up to whole pages).
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Null pointer or address out of region.
 * @retval WHAL_ETIMEOUT BSY did not clear within the configured timeout.
 * @retval WHAL_EHARDWARE Erase error reported in FLASH_SR.
 */
whal_Error whal_Stm32f0_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                    size_t dataSz);
#endif /* !WHAL_CFG_STM32F0_FLASH_DIRECT_API_MAPPING */

/*
 * @brief Set the flash wait-state latency in FLASH_ACR.LATENCY. Must be
 *        programmed before raising HCLK above the previous latency's
 *        maximum supported frequency.
 *
 * @param flashDev Flash device instance.
 * @param latency  Desired latency (LATENCY_0 or LATENCY_1).
 *
 * @retval WHAL_SUCCESS Latency updated.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Stm32f0_Flash_Ext_SetLatency(whal_Flash *flashDev,
                                              enum whal_Stm32f0_Flash_Latency latency);

#endif /* WHAL_STM32F0_FLASH_H */
