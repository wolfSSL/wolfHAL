/* stm32wb0_flash.h
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

#ifndef WHAL_STM32WB0_FLASH_H
#define WHAL_STM32WB0_FLASH_H

#include <wolfHAL/flash/flash.h>
#include <wolfHAL/timeout.h>

/**
 * @file stm32wb0_flash.h
 * @brief STM32WB0 flash driver configuration and helpers.
 *
 * The STM32WB0 flash controller uses a command-based interface that
 * differs significantly from the rest of the STM32 line:
 *   - Operations are launched by writing a single-byte opcode to
 *     COMMAND (ERASE=0x11, MASSERASE=0x22, WRITE=0x33, BURSTWRITE=0xCC,
 *     KEYWRITE=0xFF). There is no KEYR unlock sequence.
 *   - The 14-bit ADDRESS register encodes XADR[9:0]+YADR[5:0] (page +
 *     row + word) — not a byte address. Page = ADDRESS[15:9], word =
 *     ADDRESS[5:0] (1 word = 4 bytes).
 *   - DATA0-3 carry up to 4 words for BURSTWRITE; single WRITE only
 *     uses DATA0.
 *   - Operation status is tracked through IRQSTAT (CMDSTART / CMDDONE
 *     write-1-to-clear flags). Errors land in CMDERR / ILLCMD.
 *   - Main flash spans 0x10040000 onward in 2 KB pages (96 pages on
 *     WB05KZ).
 *   - One specific address can be programmed at most twice between
 *     erases — the controller enforces this by only allowing 1->0
 *     transitions.
 */

/**
 * @brief Flash device configuration.
 */
typedef struct whal_Stm32wb0_Flash_Cfg {
    size_t startAddr;        /* Flash base address (0x10040000 on WB0). */
    size_t size;             /* Flash size in bytes (depends on device). */
    whal_Timeout *timeout;
} whal_Stm32wb0_Flash_Cfg;

/**
 * @brief Flash access latency selection (CONFIG.WAIT_STATES).
 *
 * 0 WS for system clock <= 32 MHz; 1 WS for 64 MHz (RM0529 9.4.2).
 */
typedef enum whal_Stm32wb0_Flash_Latency {
    WHAL_STM32WB0_FLASH_LATENCY_0,
    WHAL_STM32WB0_FLASH_LATENCY_1,
} whal_Stm32wb0_Flash_Latency;

#ifndef WHAL_CFG_STM32WB0_FLASH_DIRECT_API_MAPPING
/**
 * @brief Driver instance for STM32WB0 flash.
 */
extern const whal_FlashDriver whal_Stm32wb0_Flash_Driver;

/**
 * @brief Platform-owned flash device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32WB0_FLASH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Flash whal_Stm32wb0_Flash_Dev;

/**
 * @brief Initialize the STM32WB0 flash controller.
 *
 * No unlock sequence is required for write/erase on WB0 — the controller
 * accepts opcodes directly. Init is a no-op kept for API consistency.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Stm32wb0_Flash_Init(whal_Flash *flashDev);

/**
 * @brief Deinitialize the STM32WB0 flash controller.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Stm32wb0_Flash_Deinit(whal_Flash *flashDev);

/**
 * @brief No-op lock (WB0 controller has no software lock register).
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Stm32wb0_Flash_Lock(whal_Flash *flashDev, size_t addr,
                                    size_t len);

/**
 * @brief No-op unlock (WB0 controller has no KEYR unlock sequence).
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Stm32wb0_Flash_Unlock(whal_Flash *flashDev, size_t addr,
                                      size_t len);

/**
 * @brief Read data from flash into a buffer (memory-mapped read).
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address in flash.
 * @param data     Destination buffer.
 * @param dataSz   Number of bytes to read.
 *
 * @retval WHAL_SUCCESS Read completed.
 * @retval WHAL_EINVAL  Null buffer or out-of-range range.
 */
whal_Error whal_Stm32wb0_Flash_Read(whal_Flash *flashDev, size_t addr,
                                    void *data, size_t dataSz);

/**
 * @brief Program flash one 32-bit word at a time via the WRITE command.
 *
 * Address and size must be 4-byte aligned. The controller enforces a
 * 1->0-only programming rule — overwriting requires an Erase first.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address in flash.
 * @param data     Buffer to program.
 * @param dataSz   Number of bytes to program.
 *
 * @retval WHAL_SUCCESS Write completed.
 * @retval WHAL_EINVAL  Misaligned, null, or out-of-range arguments.
 * @retval WHAL_ETIMEOUT Command did not complete before timeout.
 * @retval WHAL_EHARDWARE Controller reported CMDERR or ILLCMD.
 */
whal_Error whal_Stm32wb0_Flash_Write(whal_Flash *flashDev, size_t addr,
                                     const void *data, size_t dataSz);

/**
 * @brief Erase one or more 2 KB pages covering the requested range.
 *
 * @param flashDev Flash device instance.
 * @param addr     Byte address inside the first page to erase.
 * @param dataSz   Number of bytes to erase (rounded up to page size).
 *
 * @retval WHAL_SUCCESS Erase completed.
 * @retval WHAL_EINVAL  Out-of-range range.
 * @retval WHAL_ETIMEOUT Command did not complete before timeout.
 * @retval WHAL_EHARDWARE Controller reported CMDERR or ILLCMD.
 */
whal_Error whal_Stm32wb0_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                     size_t dataSz);
#endif /* !WHAL_CFG_STM32WB0_FLASH_DIRECT_API_MAPPING */

/**
 * @brief Update flash latency wait states.
 *
 * @param flashDev Flash device instance.
 * @param latency  Latency setting to apply.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Stm32wb0_Flash_Ext_SetLatency(whal_Flash *flashDev,
    enum whal_Stm32wb0_Flash_Latency latency);

#endif /* WHAL_STM32WB0_FLASH_H */
