/* stm32u5_flash.h
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

#ifndef WHAL_STM32U5_FLASH_H
#define WHAL_STM32U5_FLASH_H

#include <wolfHAL/flash/flash.h>
#include <wolfHAL/timeout.h>

/**
 * @file stm32u5_flash.h
 * @brief STM32U5 flash driver configuration.
 *
 * The STM32U5 embedded flash provides:
 * - Up to 4 MB organized in 8 KB pages, always dual-bank on production parts
 * - 128-bit (flash-word) programming, 16-byte aligned
 * - Non-secure register variants (NSCR, NSSR, NSKEYR)
 * - TrustZone support (this driver uses non-secure registers)
 *
 * Key register differences from STM32WBA:
 * - NSCR PNB field is 8 bits (PNB[7:0]) at bits 10:3 (vs 7 bits at 9:3 on WBA)
 * - NSCR adds BKER (bit 11) for dual-bank page selection
 * - NSCR adds MER1 (bit 2) and MER2 (bit 15) for per-bank mass erase
 *
 * Bank layout is part-dependent:
 *   - 1 MB U575/U585 variants: 512 KB per bank, 64 pages per bank.
 *   - 2 MB U575/U585 variants: 1 MB per bank, 128 pages per bank.
 *   - 4 MB U59x/U5Ax/U5Fx/U5Gx variants: 2 MB per bank, 256 pages per bank.
 * Bank 2 lives immediately after bank 1 in the address space and is selected
 * with BKER=1. The wolfHAL_board.h flash config provides the actual bank
 * size for the chip in use; the constants below are defaults used when
 * bankSize is 0.
 */

#define WHAL_STM32U5_FLASH_BANK_SIZE     0x00200000 /* 2 MB per bank on U5Ax */
#define WHAL_STM32U5_FLASH_PAGE_SIZE     0x2000     /* 8 KB */

/**
 * @brief STM32U5 flash driver configuration.
 */
typedef struct whal_Stm32u5_Flash_Cfg {
    size_t startAddr;          /**< Flash region start address (typ 0x08000000) */
    size_t size;               /**< Flash region size in bytes */
    size_t bankSize;           /**< Size of one bank (typ 512 KB on 1 MB part) */
    whal_Timeout *timeout;     /**< Timeout for poll loops */
} whal_Stm32u5_Flash_Cfg;

/**
 * @brief Driver instance for the STM32U5 embedded flash controller.
 */
extern const whal_FlashDriver whal_Stm32u5_Flash_Driver;

/**
 * @brief Platform-owned device singleton. Defined in the driver TU
 *        from the WHAL_CFG_STM32U5_FLASH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Flash whal_Stm32u5_Flash_Dev;

/**
 * @brief Initialize the STM32U5 flash driver.
 */
whal_Error whal_Stm32u5_Flash_Init(whal_Flash *flashDev);

/**
 * @brief Deinitialize the STM32U5 flash driver.
 */
whal_Error whal_Stm32u5_Flash_Deinit(whal_Flash *flashDev);

/**
 * @brief Re-lock the STM32U5 flash for writes (sets NSCR.LOCK).
 */
whal_Error whal_Stm32u5_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len);

/**
 * @brief Unlock the STM32U5 flash for writes via the NSKEYR sequence.
 *        Checks LOCK first to avoid double-unlock hard-fault.
 */
whal_Error whal_Stm32u5_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len);

/**
 * @brief Read `dataSz` bytes from flash at `addr` into `data`.
 */
whal_Error whal_Stm32u5_Flash_Read(whal_Flash *flashDev, size_t addr,
                                   void *data, size_t dataSz);

/**
 * @brief Program `dataSz` bytes from `data` to flash at `addr`. Address and
 *        size must be 128-bit (flash-word) aligned.
 */
whal_Error whal_Stm32u5_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz);

/**
 * @brief Erase one or more 8 KB pages covering [addr, addr+dataSz).
 *        Handles dual-bank page numbering (BKER + PNB[7:0]).
 */
whal_Error whal_Stm32u5_Flash_Erase(whal_Flash *flashDev, size_t addr, size_t dataSz);

/**
 * @brief Set the flash wait-state latency in FLASH_ACR.LATENCY.
 *        Must be programmed before raising HCLK above the previous latency's
 *        maximum supported frequency.
 */
whal_Error whal_Stm32u5_Flash_Ext_SetLatency(whal_Flash *flashDev, uint8_t latency);

#endif /* WHAL_STM32U5_FLASH_H */
