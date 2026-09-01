/* stm32f4_flash.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32F4_FLASH_DEV initializer */
#include <wolfHAL/reg.h>
#include <wolfHAL/flash/stm32f4_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/error.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

const whal_Flash whal_Stm32f4_Flash_Dev = WHAL_CFG_STM32F4_FLASH_DEV;

/*
 * STM32F4 Flash Register Definitions
 *
 * The STM32F4 flash controller manages embedded flash with variable-size
 * sectors and 32-bit word programming (at 2.7-3.6V with PSIZE=10).
 *
 * Flash interface base: 0x40023C00
 */

/* Access Control Register - configures latency and caches */
#define FLASH_ACR_REG 0x00
#define FLASH_ACR_LATENCY_Pos 0
#define FLASH_ACR_LATENCY_Msk (WHAL_BITMASK(4) << FLASH_ACR_LATENCY_Pos)

/* Key Register - unlock sequence */
#define FLASH_KEYR_REG 0x04

/* Status Register */
#define FLASH_SR_REG 0x0C
#define FLASH_SR_EOP_Pos 0
#define FLASH_SR_EOP_Msk (1UL << FLASH_SR_EOP_Pos)

#define FLASH_SR_OPERR_Pos 1
#define FLASH_SR_OPERR_Msk (1UL << FLASH_SR_OPERR_Pos)

#define FLASH_SR_WRPERR_Pos 4
#define FLASH_SR_WRPERR_Msk (1UL << FLASH_SR_WRPERR_Pos)

#define FLASH_SR_PGAERR_Pos 5
#define FLASH_SR_PGAERR_Msk (1UL << FLASH_SR_PGAERR_Pos)

#define FLASH_SR_PGPERR_Pos 6
#define FLASH_SR_PGPERR_Msk (1UL << FLASH_SR_PGPERR_Pos)

#define FLASH_SR_PGSERR_Pos 7
#define FLASH_SR_PGSERR_Msk (1UL << FLASH_SR_PGSERR_Pos)

#define FLASH_SR_RDERR_Pos 8
#define FLASH_SR_RDERR_Msk (1UL << FLASH_SR_RDERR_Pos)

#define FLASH_SR_BSY_Pos 16
#define FLASH_SR_BSY_Msk (1UL << FLASH_SR_BSY_Pos)

#define FLASH_SR_ALL_ERR (FLASH_SR_OPERR_Msk | FLASH_SR_WRPERR_Msk | \
                          FLASH_SR_PGAERR_Msk | FLASH_SR_PGPERR_Msk | \
                          FLASH_SR_PGSERR_Msk | FLASH_SR_RDERR_Msk)

/* Control Register */
#define FLASH_CR_REG 0x10
#define FLASH_CR_PG_Pos 0
#define FLASH_CR_PG_Msk (1UL << FLASH_CR_PG_Pos)

#define FLASH_CR_SER_Pos 1
#define FLASH_CR_SER_Msk (1UL << FLASH_CR_SER_Pos)

#define FLASH_CR_SNB_Pos 3
#define FLASH_CR_SNB_Msk (WHAL_BITMASK(4) << FLASH_CR_SNB_Pos)

#define FLASH_CR_PSIZE_Pos 8
#define FLASH_CR_PSIZE_Msk (WHAL_BITMASK(2) << FLASH_CR_PSIZE_Pos)

#define FLASH_CR_STRT_Pos 16
#define FLASH_CR_STRT_Msk (1UL << FLASH_CR_STRT_Pos)

#define FLASH_CR_LOCK_Pos 31
#define FLASH_CR_LOCK_Msk (1UL << FLASH_CR_LOCK_Pos)

/* PSIZE values: 00=x8, 01=x16, 10=x32, 11=x64 */
#define FLASH_PSIZE_WORD 2

/* Unlock keys */
#define FLASH_KEY1 0x45670123UL
#define FLASH_KEY2 0xCDEF89ABUL

#ifdef WHAL_CFG_STM32F4_FLASH_DIRECT_API_MAPPING
#define whal_Stm32f4_Flash_Init   whal_Flash_Init
#define whal_Stm32f4_Flash_Deinit whal_Flash_Deinit
#define whal_Stm32f4_Flash_Lock   whal_Flash_Lock
#define whal_Stm32f4_Flash_Unlock whal_Flash_Unlock
#define whal_Stm32f4_Flash_Read   whal_Flash_Read
#define whal_Stm32f4_Flash_Write  whal_Flash_Write
#define whal_Stm32f4_Flash_Erase  whal_Flash_Erase
#endif /* WHAL_CFG_STM32F4_FLASH_DIRECT_API_MAPPING */

whal_Error whal_Stm32f4_Flash_Init(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Flash_Deinit(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len)
{
    size_t base = whal_Stm32f4_Flash_Dev.base;
    (void)flashDev;
    (void)addr;
    (void)len;

    whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_LOCK_Msk,
                    whal_SetBits(FLASH_CR_LOCK_Msk, FLASH_CR_LOCK_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len)
{
    size_t base = whal_Stm32f4_Flash_Dev.base;
    (void)flashDev;
    (void)addr;
    (void)len;

    whal_Reg_Write(base, FLASH_KEYR_REG, FLASH_KEY1);
    whal_Reg_Write(base, FLASH_KEYR_REG, FLASH_KEY2);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Flash_Read(whal_Flash *flashDev, size_t addr,
                                   void *data, size_t dataSz)
{
    const whal_Stm32f4_Flash_Cfg *cfg =
        (const whal_Stm32f4_Flash_Cfg *)whal_Stm32f4_Flash_Dev.cfg;
    uint8_t *dataBuf = (uint8_t *)data;
    (void)flashDev;

    if (!data)
        return WHAL_EINVAL;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    uint8_t *flashAddr = (uint8_t *)addr;
    for (size_t i = 0; i < dataSz; ++i)
        dataBuf[i] = flashAddr[i];

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32f4_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz)
{
    const whal_Stm32f4_Flash_Cfg *cfg =
        (const whal_Stm32f4_Flash_Cfg *)whal_Stm32f4_Flash_Dev.cfg;
    size_t base = whal_Stm32f4_Flash_Dev.base;
    const uint8_t *dataBuf = (const uint8_t *)data;
    whal_Error err = WHAL_SUCCESS;
    (void)flashDev;

    if (!data)
        return WHAL_EINVAL;

    /* Address and size must be 4-byte aligned for word programming */
    if ((addr & 0x3) || (dataSz & 0x3))
        return WHAL_EINVAL;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Wait for any ongoing operation */
    err = whal_Reg_ReadPoll(base, FLASH_SR_REG, FLASH_SR_BSY_Msk,
                             0, cfg->timeout);
    if (err)
        return err;

    /* Clear all error flags */
    whal_Reg_Update(base, FLASH_SR_REG, FLASH_SR_ALL_ERR, FLASH_SR_ALL_ERR);

    /* Set programming mode with word-size parallelism */
    whal_Reg_Update(base, FLASH_CR_REG,
                    FLASH_CR_PG_Msk | FLASH_CR_PSIZE_Msk,
                    whal_SetBits(FLASH_CR_PG_Msk, FLASH_CR_PG_Pos, 1) |
                    whal_SetBits(FLASH_CR_PSIZE_Msk, FLASH_CR_PSIZE_Pos, FLASH_PSIZE_WORD));

    /* Program data in 32-bit word chunks */
    for (size_t i = 0; i < dataSz; i += 4) {
        uint32_t *flashAddr = (uint32_t *)(addr + i);
        const uint32_t *dataAddr = (const uint32_t *)(dataBuf + i);

        *flashAddr = *dataAddr;

        /* Wait for programming to complete */
        err = whal_Reg_ReadPoll(base, FLASH_SR_REG, FLASH_SR_BSY_Msk,
                                 0, cfg->timeout);
        if (err)
            goto cleanup;

        /* Check for errors */
        if (whal_Reg_Read(base, FLASH_SR_REG) & FLASH_SR_ALL_ERR) {
            whal_Reg_Update(base, FLASH_SR_REG, FLASH_SR_ALL_ERR, FLASH_SR_ALL_ERR);
            err = WHAL_EHARDWARE;
            goto cleanup;
        }
    }

cleanup:
    /* Disable programming mode */
    whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_PG_Msk, 0);

    return err;
}

whal_Error whal_Stm32f4_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                    size_t dataSz)
{
    const whal_Stm32f4_Flash_Cfg *cfg =
        (const whal_Stm32f4_Flash_Cfg *)whal_Stm32f4_Flash_Dev.cfg;
    size_t base = whal_Stm32f4_Flash_Dev.base;
    whal_Error err = WHAL_SUCCESS;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_SUCCESS;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Wait for any ongoing operation */
    err = whal_Reg_ReadPoll(base, FLASH_SR_REG, FLASH_SR_BSY_Msk,
                             0, cfg->timeout);
    if (err)
        return err;

    /* Clear all error flags */
    whal_Reg_Update(base, FLASH_SR_REG, FLASH_SR_ALL_ERR, FLASH_SR_ALL_ERR);

    /* Find and erase sectors that overlap with the address range */
    for (size_t s = 0; s < cfg->sectorCount; s++) {
        size_t sectStart = cfg->sectors[s].addr;
        size_t sectEnd = sectStart + cfg->sectors[s].size;

        /* Check if this sector overlaps with the erase range */
        if (sectStart >= addr + dataSz || sectEnd <= addr)
            continue;

        /* Set sector erase with word parallelism */
        whal_Reg_Update(base, FLASH_CR_REG,
                        FLASH_CR_SER_Msk | FLASH_CR_SNB_Msk | FLASH_CR_PSIZE_Msk,
                        whal_SetBits(FLASH_CR_SER_Msk, FLASH_CR_SER_Pos, 1) |
                        whal_SetBits(FLASH_CR_SNB_Msk, FLASH_CR_SNB_Pos, s) |
                        whal_SetBits(FLASH_CR_PSIZE_Msk, FLASH_CR_PSIZE_Pos, FLASH_PSIZE_WORD));

        /* Start erase */
        whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_STRT_Msk,
                        whal_SetBits(FLASH_CR_STRT_Msk, FLASH_CR_STRT_Pos, 1));

        /* Wait for erase to complete */
        err = whal_Reg_ReadPoll(base, FLASH_SR_REG, FLASH_SR_BSY_Msk,
                                 0, cfg->timeout);
        if (err)
            goto cleanup;

        /* Check for errors */
        if (whal_Reg_Read(base, FLASH_SR_REG) & FLASH_SR_ALL_ERR) {
            whal_Reg_Update(base, FLASH_SR_REG, FLASH_SR_ALL_ERR, FLASH_SR_ALL_ERR);
            err = WHAL_EHARDWARE;
            goto cleanup;
        }
    }

cleanup:
    /* Clear sector erase mode */
    whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_SER_Msk, 0);

    return err;
}

whal_Error whal_Stm32f4_Flash_Ext_SetLatency(whal_Flash *flashDev,
                                             enum whal_Stm32f4_Flash_Latency latency)
{
    size_t base = whal_Stm32f4_Flash_Dev.base;
    (void)flashDev;

    whal_Reg_Update(base, FLASH_ACR_REG, FLASH_ACR_LATENCY_Msk,
                    whal_SetBits(FLASH_ACR_LATENCY_Msk, FLASH_ACR_LATENCY_Pos, latency));
    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32F4_FLASH_DIRECT_API_MAPPING
const whal_FlashDriver whal_Stm32f4_Flash_Driver = {
    .Init = whal_Stm32f4_Flash_Init,
    .Deinit = whal_Stm32f4_Flash_Deinit,
    .Lock = whal_Stm32f4_Flash_Lock,
    .Unlock = whal_Stm32f4_Flash_Unlock,
    .Read = whal_Stm32f4_Flash_Read,
    .Write = whal_Stm32f4_Flash_Write,
    .Erase = whal_Stm32f4_Flash_Erase,
};
#endif /* !WHAL_CFG_STM32F4_FLASH_DIRECT_API_MAPPING */
