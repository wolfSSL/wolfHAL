/* stm32l1_flash.h
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

#ifndef WHAL_STM32L1_FLASH_H
#define WHAL_STM32L1_FLASH_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32l1_flash.h
 * @brief STM32L1 flash driver configuration.
 *
 * The STM32L1 flash uses a PECR-based unlock model:
 *   1. Unlock FLASH_PECR via FLASH_PEKEYR (PEKEY1=0x89ABCDEF, PEKEY2=0x02030405)
 *   2. Unlock program memory via FLASH_PRGKEYR (PRGKEY1=0x8C9DAEBF, PRGKEY2=0x13141516)
 *
 * Register layout:
 *   FLASH_ACR     = 0x00  (LATENCY, ACC64, PRFTEN, RUN_PD, SLEEP_PD)
 *   FLASH_PECR    = 0x04  (PELOCK, PRGLOCK, OPTLOCK, PROG, ERASE, FPRG, DATA, FTDW)
 *   FLASH_PDKEYR  = 0x08
 *   FLASH_PEKEYR  = 0x0C
 *   FLASH_PRGKEYR = 0x10
 *   FLASH_OPTKEYR = 0x14
 *   FLASH_SR      = 0x18  (BSY, EOP, ENDHV, READY, WRPERR, PGAERR, SIZERR, OPTVERR)
 *   FLASH_OBR     = 0x1C
 *   FLASH_WRPR1   = 0x20
 *
 * Programming:
 *   - Write alignment: 4 bytes (word)
 *   - Erase granularity: 256 bytes (page)
 *   - Fast word write: unlock PECR + PRGKEYR, write 32-bit word to flash address
 *   - Page erase: set ERASE+PROG in PECR, write 0x00000000 to first word of page
 */

/*
 * @brief STM32L1 flash driver configuration.
 */
typedef struct whal_Stm32l1_Flash_Cfg {
    size_t startAddr;          /* Flash region start address */
    size_t size;               /* Flash region size in bytes */
    whal_Timeout *timeout;     /* Optional timeout for poll loops */
} whal_Stm32l1_Flash_Cfg;

/*
 * @brief Flash access latency values for STM32L1.
 *
 * 0 WS: HCLK <= 16 MHz (voltage range 1 / 2), HCLK <= 8 MHz (range 3)
 * 1 WS: 16 < HCLK <= 32 MHz (range 1), 8 < HCLK <= 16 MHz (range 2/3)
 */
typedef enum {
    WHAL_STM32L1_FLASH_LATENCY_0 = 0,
    WHAL_STM32L1_FLASH_LATENCY_1 = 1,
} whal_Stm32l1_Flash_Latency;

#ifndef WHAL_CFG_STM32L1_FLASH_DIRECT_API_MAPPING
/*
 * @brief Driver instance for the STM32L1 embedded flash controller.
 */
extern const whal_FlashDriver whal_Stm32l1_Flash_Driver;
/*
 * @brief Platform-owned device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32L1_FLASH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Flash whal_Stm32l1_Flash_Dev;

/*
 * @brief Initialize the STM32L1 flash driver. Validates cfg.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Driver is ready.
 * @retval WHAL_EINVAL  Null pointer or missing cfg.
 */
whal_Error whal_Stm32l1_Flash_Init(whal_Flash *flashDev);

/*
 * @brief Deinitialize the STM32L1 flash driver.
 *
 * @param flashDev Flash device instance.
 *
 * @retval WHAL_SUCCESS Driver is deinitialized.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Stm32l1_Flash_Deinit(whal_Flash *flashDev);

/*
 * @brief Re-lock program memory (sets PRGLOCK in FLASH_PECR).
 *
 * @param flashDev Flash device instance.
 * @param addr     Range start (ignored — flash is globally locked).
 * @param len      Range length (ignored).
 *
 * @retval WHAL_SUCCESS Flash is locked.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Stm32l1_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len);

/*
 * @brief Unlock program memory via PEKEYR + PRGKEYR sequences.
 *
 * @param flashDev Flash device instance.
 * @param addr     Range start (ignored — flash is globally unlocked).
 * @param len      Range length (ignored).
 *
 * @retval WHAL_SUCCESS Flash is unlocked.
 * @retval WHAL_EINVAL  Null pointer.
 * @retval WHAL_EHARDWARE Key sequence rejected.
 */
whal_Error whal_Stm32l1_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len);

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
whal_Error whal_Stm32l1_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                                   size_t dataSz);

/*
 * @brief Program `dataSz` bytes from `data` to flash at `addr`. Address and
 *        size must be 4-byte (word) aligned.
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
whal_Error whal_Stm32l1_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz);

/*
 * @brief Erase one or more 256-byte pages covering [addr, addr+dataSz).
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
whal_Error whal_Stm32l1_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                    size_t dataSz);
#endif /* !WHAL_CFG_STM32L1_FLASH_DIRECT_API_MAPPING */

/*
 * @brief Set flash access latency (FLASH_ACR.LATENCY).
 *
 * Enables 64-bit access (ACC64) and prefetch (PRFTEN) as required by RM0038
 * §3.9.1 before programming the LATENCY bit, and polls until LATENCY matches
 * the requested value. Must be called by the board before increasing SYSCLK
 * beyond 16 MHz.
 *
 * @param latency Desired latency (LATENCY_0 or LATENCY_1).
 *
 * @retval WHAL_SUCCESS Latency updated.
 */
whal_Error whal_Stm32l1_Flash_Ext_SetLatency(whal_Stm32l1_Flash_Latency latency);

#endif /* WHAL_STM32L1_FLASH_H */
