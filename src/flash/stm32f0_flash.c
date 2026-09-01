/* stm32f0_flash.c
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

#include <stdint.h>
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32F0_FLASH_DEV initializer */
#include <wolfHAL/reg.h>
#include <wolfHAL/flash/stm32f0_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/error.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

const whal_Flash whal_Stm32f0_Flash_Dev = WHAL_CFG_STM32F0_FLASH_DEV;

#define FLASH_ACR_REG 0x00
#define FLASH_ACR_LATENCY_Pos 0
#define FLASH_ACR_LATENCY_Msk (WHAL_BITMASK(3) << FLASH_ACR_LATENCY_Pos)

#define FLASH_KEYR_REG 0x04
#define FLASH_KEYR_KEY_Msk (~0UL)

#define FLASH_SR_REG 0x0C
#define FLASH_SR_BSY_Pos 0
#define FLASH_SR_BSY_Msk (1UL << FLASH_SR_BSY_Pos)
#define FLASH_SR_PGERR_Pos 2
#define FLASH_SR_PGERR_Msk (1UL << FLASH_SR_PGERR_Pos)
#define FLASH_SR_WRPRTERR_Pos 4
#define FLASH_SR_WRPRTERR_Msk (1UL << FLASH_SR_WRPRTERR_Pos)
#define FLASH_SR_EOP_Pos 5
#define FLASH_SR_EOP_Msk (1UL << FLASH_SR_EOP_Pos)

#define FLASH_SR_ALL_ERR (FLASH_SR_PGERR_Msk | FLASH_SR_WRPRTERR_Msk)

#define FLASH_CR_REG 0x10
#define FLASH_CR_PG_Pos 0
#define FLASH_CR_PG_Msk (1UL << FLASH_CR_PG_Pos)
#define FLASH_CR_PER_Pos 1
#define FLASH_CR_PER_Msk (1UL << FLASH_CR_PER_Pos)
#define FLASH_CR_STRT_Pos 6
#define FLASH_CR_STRT_Msk (1UL << FLASH_CR_STRT_Pos)
#define FLASH_CR_LOCK_Pos 7
#define FLASH_CR_LOCK_Msk (1UL << FLASH_CR_LOCK_Pos)

#define FLASH_AR_REG 0x14

#ifdef WHAL_CFG_STM32F0_FLASH_DIRECT_API_MAPPING
#define whal_Stm32f0_Flash_Init   whal_Flash_Init
#define whal_Stm32f0_Flash_Deinit whal_Flash_Deinit
#define whal_Stm32f0_Flash_Lock   whal_Flash_Lock
#define whal_Stm32f0_Flash_Unlock whal_Flash_Unlock
#define whal_Stm32f0_Flash_Read   whal_Flash_Read
#define whal_Stm32f0_Flash_Write  whal_Flash_Write
#define whal_Stm32f0_Flash_Erase  whal_Flash_Erase
#endif

whal_Error whal_Stm32f0_Flash_Init(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f0_Flash_Deinit(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f0_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len)
{
    size_t base = whal_Stm32f0_Flash_Dev.base;
    (void)flashDev;
    (void)addr;
    (void)len;

    whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_LOCK_Msk,
                    whal_SetBits(FLASH_CR_LOCK_Msk, FLASH_CR_LOCK_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f0_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len)
{
    size_t base = whal_Stm32f0_Flash_Dev.base;
    (void)flashDev;
    (void)addr;
    (void)len;

    whal_Reg_Update(base, FLASH_KEYR_REG,
                    FLASH_KEYR_KEY_Msk, 0x45670123);
    whal_Reg_Update(base, FLASH_KEYR_REG,
                    FLASH_KEYR_KEY_Msk, 0xCDEF89AB);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f0_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                                   size_t dataSz)
{
    const whal_Stm32f0_Flash_Cfg *cfg =
        (const whal_Stm32f0_Flash_Cfg *)whal_Stm32f0_Flash_Dev.cfg;
    uint8_t *dataBuf = (uint8_t *)data;
    (void)flashDev;

    if (!data)
        return WHAL_EINVAL;

    if (dataSz == 0)
        return WHAL_SUCCESS;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    uint8_t *flashAddr = (uint8_t *)addr;
    for (size_t i = 0; i < dataSz; ++i)
        dataBuf[i] = flashAddr[i];

    return WHAL_SUCCESS;
}

static whal_Error whal_Stm32f0_Flash_WriteOrErase(whal_Flash *flashDev,
                                                   size_t addr,
                                                   const uint8_t *data,
                                                   size_t dataSz, uint8_t write)
{
    const whal_Stm32f0_Flash_Cfg *cfg =
        (const whal_Stm32f0_Flash_Cfg *)whal_Stm32f0_Flash_Dev.cfg;
    size_t base = whal_Stm32f0_Flash_Dev.base;
    size_t bsy;
    (void)flashDev;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Write requires 2-byte alignment (16-bit half-word programming) */
    if (write && ((addr & 0x1) || (dataSz & 0x1)))
        return WHAL_EINVAL;

    if (!write && dataSz == 0)
        return WHAL_SUCCESS;

    whal_Reg_Get(base, FLASH_SR_REG,
                 FLASH_SR_BSY_Msk, FLASH_SR_BSY_Pos, &bsy);
    if (bsy)
        return WHAL_ENOTREADY;

    /* Clear error flags */
    whal_Reg_Update(base, FLASH_SR_REG, FLASH_SR_ALL_ERR, 0xffffffff);

    whal_Error err = WHAL_SUCCESS;

    if (write) {
        /* Enable programming */
        whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_PG_Msk,
                        whal_SetBits(FLASH_CR_PG_Msk, FLASH_CR_PG_Pos, 1));

        /* Program in 16-bit half-words */
        for (size_t i = 0; i < dataSz; i += 2) {
            volatile uint16_t *flashAddr = (volatile uint16_t *)(addr + i);
            uint16_t hw = (uint16_t)data[i] | ((uint16_t)data[i + 1] << 8);

            *flashAddr = hw;

            err = whal_Reg_ReadPoll(base, FLASH_SR_REG,
                                    FLASH_SR_BSY_Msk, 0, cfg->timeout);
            if (err)
                goto cleanup;
        }
    } else {
        /* Calculate page range (2 KB per page) */
        size_t startPage = (addr - cfg->startAddr) >> 11;
        size_t endPage = ((addr - cfg->startAddr) + dataSz - 1) >> 11;

        for (size_t page = startPage; page <= endPage; ++page) {
            /* Enable page erase */
            whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_PER_Msk,
                            whal_SetBits(FLASH_CR_PER_Msk, FLASH_CR_PER_Pos, 1));

            /* Write page start address to AR */
            whal_Reg_Write(base, FLASH_AR_REG,
                           cfg->startAddr + (page << 11));

            /* Start erase */
            whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_STRT_Msk,
                            whal_SetBits(FLASH_CR_STRT_Msk, FLASH_CR_STRT_Pos, 1));

            err = whal_Reg_ReadPoll(base, FLASH_SR_REG,
                                    FLASH_SR_BSY_Msk, 0, cfg->timeout);
            if (err)
                goto cleanup;
        }

        /* Disable page erase */
        whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_PER_Msk,
                        whal_SetBits(FLASH_CR_PER_Msk, FLASH_CR_PER_Pos, 0));
    }

cleanup:
    whal_Reg_Update(base, FLASH_CR_REG,
                    FLASH_CR_PG_Msk | FLASH_CR_PER_Msk, 0);

    return err;
}

whal_Error whal_Stm32f0_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz)
{
    return whal_Stm32f0_Flash_WriteOrErase(flashDev, addr,
                                           (const uint8_t *)data, dataSz, 1);
}

whal_Error whal_Stm32f0_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                    size_t dataSz)
{
    return whal_Stm32f0_Flash_WriteOrErase(flashDev, addr, NULL, dataSz, 0);
}

whal_Error whal_Stm32f0_Flash_Ext_SetLatency(whal_Flash *flashDev,
                                              enum whal_Stm32f0_Flash_Latency latency)
{
    size_t base = whal_Stm32f0_Flash_Dev.base;
    (void)flashDev;

    whal_Reg_Update(base, FLASH_ACR_REG,
                    FLASH_ACR_LATENCY_Msk, latency);
    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32F0_FLASH_DIRECT_API_MAPPING
const whal_FlashDriver whal_Stm32f0_Flash_Driver = {
    .Init = whal_Stm32f0_Flash_Init,
    .Deinit = whal_Stm32f0_Flash_Deinit,
    .Lock = whal_Stm32f0_Flash_Lock,
    .Unlock = whal_Stm32f0_Flash_Unlock,
    .Read = whal_Stm32f0_Flash_Read,
    .Write = whal_Stm32f0_Flash_Write,
    .Erase = whal_Stm32f0_Flash_Erase,
};
#endif
