/* stm32h5_flash.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32H5_FLASH_DEV initializer */
#include <wolfHAL/reg.h>
#include <wolfHAL/flash/stm32h5_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/error.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

const whal_Flash whal_Stm32h5_Flash_Dev = WHAL_CFG_STM32H5_FLASH_DEV;

/*
 * STM32H5 Flash Register Definitions
 *
 * The STM32H5 flash controller manages dual-bank embedded flash with 8 KB
 * sectors and 128-bit (flash-word) programming. Non-secure register variants
 * (NS prefix) are used throughout.
 */

/* Access Control Register */
#define FLASH_ACR_REG 0x000

/* Non-Secure Key Register - unlock sequence */
#define FLASH_NSKEYR_REG 0x004

/* Non-Secure Status Register */
#define FLASH_NSSR_REG 0x020
#define FLASH_NSSR_BSY_Pos 0
#define FLASH_NSSR_BSY_Msk (1UL << FLASH_NSSR_BSY_Pos)

#define FLASH_NSSR_WBNE_Pos 1
#define FLASH_NSSR_WBNE_Msk (1UL << FLASH_NSSR_WBNE_Pos)

#define FLASH_NSSR_EOP_Pos 16
#define FLASH_NSSR_EOP_Msk (1UL << FLASH_NSSR_EOP_Pos)

#define FLASH_NSSR_WRPERR_Pos 17
#define FLASH_NSSR_WRPERR_Msk (1UL << FLASH_NSSR_WRPERR_Pos)

#define FLASH_NSSR_PGSERR_Pos 18
#define FLASH_NSSR_PGSERR_Msk (1UL << FLASH_NSSR_PGSERR_Pos)

#define FLASH_NSSR_STRBERR_Pos 19
#define FLASH_NSSR_STRBERR_Msk (1UL << FLASH_NSSR_STRBERR_Pos)

#define FLASH_NSSR_INCERR_Pos 20
#define FLASH_NSSR_INCERR_Msk (1UL << FLASH_NSSR_INCERR_Pos)

#define FLASH_NSSR_ALL_ERR (FLASH_NSSR_WRPERR_Msk | FLASH_NSSR_PGSERR_Msk | \
                            FLASH_NSSR_STRBERR_Msk | FLASH_NSSR_INCERR_Msk)

/* Non-Secure Control Register */
#define FLASH_NSCR_REG 0x028
#define FLASH_NSCR_LOCK_Pos 0
#define FLASH_NSCR_LOCK_Msk (1UL << FLASH_NSCR_LOCK_Pos)

#define FLASH_NSCR_PG_Pos 1
#define FLASH_NSCR_PG_Msk (1UL << FLASH_NSCR_PG_Pos)

#define FLASH_NSCR_SER_Pos 2
#define FLASH_NSCR_SER_Msk (1UL << FLASH_NSCR_SER_Pos)

#define FLASH_NSCR_BER_Pos 3
#define FLASH_NSCR_BER_Msk (1UL << FLASH_NSCR_BER_Pos)

#define FLASH_NSCR_FW_Pos 4
#define FLASH_NSCR_FW_Msk (1UL << FLASH_NSCR_FW_Pos)

#define FLASH_NSCR_STRT_Pos 5
#define FLASH_NSCR_STRT_Msk (1UL << FLASH_NSCR_STRT_Pos)

#define FLASH_NSCR_SNB_Pos 6
#define FLASH_NSCR_SNB_Msk (WHAL_BITMASK(7) << FLASH_NSCR_SNB_Pos)

#define FLASH_NSCR_BKSEL_Pos 31
#define FLASH_NSCR_BKSEL_Msk (1UL << FLASH_NSCR_BKSEL_Pos)

/* Non-Secure Clear Control Register - write 1 to clear status flags */
#define FLASH_NSCCR_REG 0x030
#define FLASH_NSCCR_CLR_EOP_Pos 16
#define FLASH_NSCCR_CLR_EOP_Msk (1UL << FLASH_NSCCR_CLR_EOP_Pos)

#define FLASH_NSCCR_CLR_WRPERR_Pos 17
#define FLASH_NSCCR_CLR_WRPERR_Msk (1UL << FLASH_NSCCR_CLR_WRPERR_Pos)

#define FLASH_NSCCR_CLR_PGSERR_Pos 18
#define FLASH_NSCCR_CLR_PGSERR_Msk (1UL << FLASH_NSCCR_CLR_PGSERR_Pos)

#define FLASH_NSCCR_CLR_STRBERR_Pos 19
#define FLASH_NSCCR_CLR_STRBERR_Msk (1UL << FLASH_NSCCR_CLR_STRBERR_Pos)

#define FLASH_NSCCR_CLR_INCERR_Pos 20
#define FLASH_NSCCR_CLR_INCERR_Msk (1UL << FLASH_NSCCR_CLR_INCERR_Pos)

#define FLASH_NSCCR_CLR_ALL (FLASH_NSCCR_CLR_EOP_Msk | \
                             FLASH_NSCCR_CLR_WRPERR_Msk | \
                             FLASH_NSCCR_CLR_PGSERR_Msk | \
                             FLASH_NSCCR_CLR_STRBERR_Msk | \
                             FLASH_NSCCR_CLR_INCERR_Msk)

/* Unlock keys (same as STM32WB) */
#define FLASH_KEY1 0x45670123UL
#define FLASH_KEY2 0xCDEF89ABUL

/* Sector size: 8 KB */
#define FLASH_SECTOR_SIZE 0x2000
#define FLASH_SECTOR_SHIFT 13

/* Sectors per bank */
#define FLASH_SECTORS_PER_BANK 128

/* Bank size: 1 MB */
#define FLASH_BANK_SIZE (FLASH_SECTORS_PER_BANK * FLASH_SECTOR_SIZE)

#ifdef WHAL_CFG_STM32H5_FLASH_DIRECT_API_MAPPING
#define whal_Stm32h5_Flash_Init   whal_Flash_Init
#define whal_Stm32h5_Flash_Deinit whal_Flash_Deinit
#define whal_Stm32h5_Flash_Lock   whal_Flash_Lock
#define whal_Stm32h5_Flash_Unlock whal_Flash_Unlock
#define whal_Stm32h5_Flash_Read   whal_Flash_Read
#define whal_Stm32h5_Flash_Write  whal_Flash_Write
#define whal_Stm32h5_Flash_Erase  whal_Flash_Erase
#endif /* WHAL_CFG_STM32H5_FLASH_DIRECT_API_MAPPING */

whal_Error whal_Stm32h5_Flash_Init(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32h5_Flash_Deinit(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32h5_Flash_Lock(whal_Flash *flashDev, size_t addr,
                                   size_t len)
{
    size_t base = whal_Stm32h5_Flash_Dev.base;
    (void)flashDev;
    (void)addr;
    (void)len;

    whal_Reg_Update(base, FLASH_NSCR_REG, FLASH_NSCR_LOCK_Msk,
                    whal_SetBits(FLASH_NSCR_LOCK_Msk, FLASH_NSCR_LOCK_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32h5_Flash_Unlock(whal_Flash *flashDev, size_t addr,
                                     size_t len)
{
    size_t base = whal_Stm32h5_Flash_Dev.base;
    (void)flashDev;
    (void)addr;
    (void)len;

    whal_Reg_Write(base, FLASH_NSKEYR_REG, FLASH_KEY1);
    whal_Reg_Write(base, FLASH_NSKEYR_REG, FLASH_KEY2);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32h5_Flash_Read(whal_Flash *flashDev, size_t addr,
                                   void *data, size_t dataSz)
{
    const whal_Stm32h5_Flash_Cfg *cfg =
        (const whal_Stm32h5_Flash_Cfg *)whal_Stm32h5_Flash_Dev.cfg;
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

static whal_Error WaitNotBusy(size_t base, whal_Timeout *timeout)
{
    return whal_Reg_ReadPoll(base, FLASH_NSSR_REG, FLASH_NSSR_BSY_Msk,
                             0, timeout);
}

static whal_Error CheckErrors(size_t base)
{
    size_t sr = whal_Reg_Read(base, FLASH_NSSR_REG);
    if (sr & FLASH_NSSR_ALL_ERR) {
        /* Clear all error flags */
        whal_Reg_Write(base, FLASH_NSCCR_REG, FLASH_NSCCR_CLR_ALL);
        return WHAL_EHARDWARE;
    }
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32h5_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz)
{
    const whal_Stm32h5_Flash_Cfg *cfg =
        (const whal_Stm32h5_Flash_Cfg *)whal_Stm32h5_Flash_Dev.cfg;
    size_t base = whal_Stm32h5_Flash_Dev.base;
    const uint8_t *dataBuf = (const uint8_t *)data;
    whal_Error err;
    (void)flashDev;

    if (!data)
        return WHAL_EINVAL;

    /* Address and size must be 16-byte aligned (128-bit flash-word) */
    if ((addr & 0xF) || (dataSz & 0xF))
        return WHAL_EINVAL;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Wait for any ongoing operation */
    err = WaitNotBusy(base, cfg->timeout);
    if (err)
        return err;

    /* Clear all error and status flags */
    whal_Reg_Write(base, FLASH_NSCCR_REG, FLASH_NSCCR_CLR_ALL);

    /* Enable programming */
    whal_Reg_Update(base, FLASH_NSCR_REG, FLASH_NSCR_PG_Msk,
                    whal_SetBits(FLASH_NSCR_PG_Msk, FLASH_NSCR_PG_Pos, 1));

    /* Program data in 128-bit (16 byte) flash-word chunks */
    for (size_t i = 0; i < dataSz; i += 16) {
        uint32_t *flashAddr = (uint32_t *)(addr + i);
        const uint32_t *dataAddr = (const uint32_t *)(dataBuf + i);

        flashAddr[0] = dataAddr[0];
        flashAddr[1] = dataAddr[1];
        flashAddr[2] = dataAddr[2];
        flashAddr[3] = dataAddr[3];

        err = WaitNotBusy(base, cfg->timeout);
        if (err)
            goto cleanup;

        err = CheckErrors(base);
        if (err)
            goto cleanup;
    }

cleanup:
    /* Disable programming */
    whal_Reg_Update(base, FLASH_NSCR_REG, FLASH_NSCR_PG_Msk, 0);

    return err;
}

whal_Error whal_Stm32h5_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                    size_t dataSz)
{
    const whal_Stm32h5_Flash_Cfg *cfg =
        (const whal_Stm32h5_Flash_Cfg *)whal_Stm32h5_Flash_Dev.cfg;
    size_t base = whal_Stm32h5_Flash_Dev.base;
    whal_Error err;
    size_t offset;
    size_t startSector, endSector;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_SUCCESS;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Wait for any ongoing operation */
    err = WaitNotBusy(base, cfg->timeout);
    if (err)
        return err;

    /* Clear all error and status flags */
    whal_Reg_Write(base, FLASH_NSCCR_REG, FLASH_NSCCR_CLR_ALL);

    offset = addr - cfg->startAddr;
    startSector = offset >> FLASH_SECTOR_SHIFT;
    endSector = (offset + dataSz - 1) >> FLASH_SECTOR_SHIFT;

    for (size_t sector = startSector; sector <= endSector; sector++) {
        size_t bank = (sector >= FLASH_SECTORS_PER_BANK) ? 1 : 0;
        size_t sectorInBank = sector - (bank * FLASH_SECTORS_PER_BANK);

        /* Configure and start sector erase in one write */
        whal_Reg_Update(base, FLASH_NSCR_REG,
                        FLASH_NSCR_SER_Msk | FLASH_NSCR_SNB_Msk |
                        FLASH_NSCR_BKSEL_Msk | FLASH_NSCR_STRT_Msk,
                        whal_SetBits(FLASH_NSCR_SER_Msk, FLASH_NSCR_SER_Pos, 1) |
                        whal_SetBits(FLASH_NSCR_SNB_Msk, FLASH_NSCR_SNB_Pos,
                                     sectorInBank) |
                        whal_SetBits(FLASH_NSCR_BKSEL_Msk, FLASH_NSCR_BKSEL_Pos,
                                     bank) |
                        whal_SetBits(FLASH_NSCR_STRT_Msk, FLASH_NSCR_STRT_Pos, 1));

        err = WaitNotBusy(base, cfg->timeout);
        if (err)
            goto cleanup;

        err = CheckErrors(base);
        if (err)
            goto cleanup;
    }

cleanup:
    /* Clear sector erase mode */
    whal_Reg_Update(base, FLASH_NSCR_REG, FLASH_NSCR_SER_Msk, 0);

    return err;
}

#define FLASH_ACR_LATENCY_WRHIGHFREQ_Msk 0x3FUL

whal_Error whal_Stm32h5_Flash_Ext_SetLatency(whal_Flash *flashDev,
                                             uint8_t latency)
{
    size_t base = whal_Stm32h5_Flash_Dev.base;
    (void)flashDev;

    whal_Reg_Update(base, FLASH_ACR_REG,
                    FLASH_ACR_LATENCY_WRHIGHFREQ_Msk, latency);

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32H5_FLASH_DIRECT_API_MAPPING
const whal_FlashDriver whal_Stm32h5_Flash_Driver = {
    .Init = whal_Stm32h5_Flash_Init,
    .Deinit = whal_Stm32h5_Flash_Deinit,
    .Lock = whal_Stm32h5_Flash_Lock,
    .Unlock = whal_Stm32h5_Flash_Unlock,
    .Read = whal_Stm32h5_Flash_Read,
    .Write = whal_Stm32h5_Flash_Write,
    .Erase = whal_Stm32h5_Flash_Erase,
};
#endif /* !WHAL_CFG_STM32H5_FLASH_DIRECT_API_MAPPING */
