/* stm32c0_flash.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32C0_FLASH_DEV initializer */
#include <wolfHAL/reg.h>
#include <wolfHAL/flash/stm32c0_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/error.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

const whal_Flash whal_Stm32c0_Flash_Dev = WHAL_CFG_STM32C0_FLASH_DEV;

/*
 * STM32C0 Flash Register Definitions
 *
 * The flash controller manages embedded flash memory operations including
 * programming, erasing, and access control. Flash is organized in 2 KB pages.
 */

/* Access Control Register - configures latency and caches */
#define FLASH_ACR_REG 0x00
#define FLASH_ACR_LATENCY_Pos 0 /* Wait states (0-1) */
#define FLASH_ACR_LATENCY_Msk (WHAL_BITMASK(3) << FLASH_ACR_LATENCY_Pos)

/* Key Register - unlock sequence for write operations */
#define FLASH_KEYR_REG 0x08
#define FLASH_KEYR_KEY_Msk (~0UL)

/* Status Register - operation status and error flags */
#define FLASH_SR_REG 0x10
#define FLASH_SR_EOP_Pos 0       /* End of operation */
#define FLASH_SR_EOP_Msk (1UL << FLASH_SR_EOP_Pos)

#define FLASH_SR_OP_ERR_Pos 1    /* Operation error */
#define FLASH_SR_OP_ERR_Msk (1UL << FLASH_SR_OP_ERR_Pos)

#define FLASH_SR_PROG_ERR_Pos 3  /* Programming error */
#define FLASH_SR_PROG_ERR_Msk (1UL << FLASH_SR_PROG_ERR_Pos)

#define FLASH_SR_WRP_ERR_Pos 4   /* Write protection error */
#define FLASH_SR_WRP_ERR_Msk (1UL << FLASH_SR_WRP_ERR_Pos)

#define FLASH_SR_PGA_ERR_Pos 5   /* Programming alignment error */
#define FLASH_SR_PGA_ERR_Msk (1UL << FLASH_SR_PGA_ERR_Pos)

#define FLASH_SR_SIZ_ERR_Pos 6   /* Size error */
#define FLASH_SR_SIZ_ERR_Msk (1UL << FLASH_SR_SIZ_ERR_Pos)

#define FLASH_SR_PGS_ERR_Pos 7   /* Programming sequence error */
#define FLASH_SR_PGS_ERR_Msk (1UL << FLASH_SR_PGS_ERR_Pos)

#define FLASH_SR_MISS_ERR_Pos 8  /* Fast programming miss error */
#define FLASH_SR_MISS_ERR_Msk (1UL << FLASH_SR_MISS_ERR_Pos)

#define FLASH_SR_FAST_ERR_Pos 9  /* Fast programming error */
#define FLASH_SR_FAST_ERR_Msk (1UL << FLASH_SR_FAST_ERR_Pos)

#define FLASH_SR_RD_ERR_Pos 14   /* Read protection error */
#define FLASH_SR_RD_ERR_Msk (1UL << FLASH_SR_RD_ERR_Pos)

#define FLASH_SR_OPTV_ERR_Pos 15 /* Option validity error */
#define FLASH_SR_OPTV_ERR_Msk (1UL << FLASH_SR_OPTV_ERR_Pos)

#define FLASH_SR_BSY_Pos 16      /* Busy flag */
#define FLASH_SR_BSY_Msk (1UL << FLASH_SR_BSY_Pos)

#define FLASH_SR_CFGBSY_Pos 18   /* Configuration busy */
#define FLASH_SR_CFGBSY_Msk (1UL << FLASH_SR_CFGBSY_Pos)

/* Combined mask for all error flags */
#define FLASH_SR_ALL_ERR (FLASH_SR_OP_ERR_Msk | FLASH_SR_PROG_ERR_Msk | FLASH_SR_WRP_ERR_Msk | \
                             FLASH_SR_PGA_ERR_Msk | FLASH_SR_SIZ_ERR_Msk | FLASH_SR_PGS_ERR_Msk | \
                             FLASH_SR_MISS_ERR_Msk | FLASH_SR_FAST_ERR_Msk | FLASH_SR_RD_ERR_Msk | \
                             FLASH_SR_OPTV_ERR_Msk)

/* Control Register - enables operations and selects pages */
#define FLASH_CR_REG 0x14
#define FLASH_CR_PG_Pos 0            /* Programming enable */
#define FLASH_CR_PG_Msk (1UL << FLASH_CR_PG_Pos)

#define FLASH_CR_PER_Pos 1           /* Page erase enable */
#define FLASH_CR_PER_Msk (1UL << FLASH_CR_PER_Pos)

#define FLASH_CR_PNB_Pos 3 /* Page number for erase (7 bits for C0) */
#define FLASH_CR_PNB_Msk (WHAL_BITMASK(7) << FLASH_CR_PNB_Pos)

#define FLASH_CR_STRT_Pos 16         /* Start erase operation */
#define FLASH_CR_STRT_Msk (1UL << FLASH_CR_STRT_Pos)

#define FLASH_CR_LOCK_Pos 31         /* Lock flash control */
#define FLASH_CR_LOCK_Msk (1UL << FLASH_CR_LOCK_Pos)

#ifdef WHAL_CFG_STM32C0_FLASH_DIRECT_API_MAPPING
#define whal_Stm32c0_Flash_Init   whal_Flash_Init
#define whal_Stm32c0_Flash_Deinit whal_Flash_Deinit
#define whal_Stm32c0_Flash_Lock   whal_Flash_Lock
#define whal_Stm32c0_Flash_Unlock whal_Flash_Unlock
#define whal_Stm32c0_Flash_Read   whal_Flash_Read
#define whal_Stm32c0_Flash_Write  whal_Flash_Write
#define whal_Stm32c0_Flash_Erase  whal_Flash_Erase
#endif /* WHAL_CFG_STM32C0_FLASH_DIRECT_API_MAPPING */

whal_Error whal_Stm32c0_Flash_Init(whal_Flash *flashDev)
{
    (void)flashDev;

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32c0_Flash_Deinit(whal_Flash *flashDev)
{
    (void)flashDev;

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32c0_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len)
{
    size_t base = whal_Stm32c0_Flash_Dev.base;
    (void)flashDev;
    (void)addr;
    (void)len;

    /* Setting LOCK bit prevents further flash modifications until next unlock */
    whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_LOCK_Msk,
                    whal_SetBits(FLASH_CR_LOCK_Msk, FLASH_CR_LOCK_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32c0_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len)
{
    size_t base = whal_Stm32c0_Flash_Dev.base;
    (void)flashDev;
    (void)addr;
    (void)len;

    /*
     * Unlock sequence: write KEY1 then KEY2 to KEYR register.
     * Incorrect sequence or order will trigger a bus error.
     */
    whal_Reg_Update(base, FLASH_KEYR_REG, FLASH_KEYR_KEY_Msk, 0x45670123);
    whal_Reg_Update(base, FLASH_KEYR_REG, FLASH_KEYR_KEY_Msk, 0xCDEF89AB);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32c0_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                             size_t dataSz)
{
    const whal_Stm32c0_Flash_Cfg *cfg =
        (const whal_Stm32c0_Flash_Cfg *)whal_Stm32c0_Flash_Dev.cfg;
    uint8_t *dataBuf = (uint8_t *)data;
    (void)flashDev;

    if (!data)
        return WHAL_EINVAL;

    if (dataSz == 0)
        return WHAL_SUCCESS;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Flash is memory-mapped, so reading is a simple memory copy */
    uint8_t *flashAddr = (uint8_t *)addr;
    for (size_t i = 0; i < dataSz; ++i) {
        dataBuf[i] = flashAddr[i];
    }
    return WHAL_SUCCESS;
}

/*
 * Internal helper for write and erase operations.
 *
 * For write (write=1): Programs data in 64-bit (8 byte) chunks.
 * For erase (write=0): Erases 2 KB pages covering the address range.
 */
static whal_Error whal_Stm32c0_Flash_WriteOrErase(whal_Flash *flashDev, size_t addr, const uint8_t *data,
                                            size_t dataSz, uint8_t write)
{
    const whal_Stm32c0_Flash_Cfg *cfg =
        (const whal_Stm32c0_Flash_Cfg *)whal_Stm32c0_Flash_Dev.cfg;
    size_t base = whal_Stm32c0_Flash_Dev.base;
    size_t bsy;
    (void)flashDev;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Write requires 8-byte alignment (64-bit double-word programming) */
    if (write && ((addr & 0x7) || (dataSz & 0x7)))
        return WHAL_EINVAL;

    if (!write && dataSz == 0)
        return WHAL_SUCCESS;

    /* Check if flash is busy */
    whal_Reg_Get(base, FLASH_SR_REG, FLASH_SR_BSY_Msk, FLASH_SR_BSY_Pos, &bsy);

    if (bsy) {
        return WHAL_ENOTREADY;
    }

    /* Clear all error flags by writing 1 to each */
    whal_Reg_Update(base, FLASH_SR_REG, FLASH_SR_ALL_ERR, 0xffffffff);

    whal_Error err = WHAL_SUCCESS;

    if (write) {
        /* Enable flash programming mode */
        whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_PG_Msk, 1);

        /* Program data in 64-bit (8 byte) double-word chunks */
        for (size_t i = 0; i < dataSz; i += 8) {
            uint32_t *flashAddr = (uint32_t *)(addr + i);
            uint32_t *dataAddr = (uint32_t *)(data + i);

            /* Write both 32-bit words to trigger the 64-bit programming */
            flashAddr[0] = dataAddr[0];
            flashAddr[1] = dataAddr[1];

            /* Wait for programming to complete */
            err = whal_Reg_ReadPoll(base, FLASH_SR_REG,
                                    FLASH_SR_CFGBSY_Msk, 0, cfg->timeout);
            if (err)
                goto cleanup;
        }
    }
    else {
        /* Calculate page range to erase (2 KB per page) */
        size_t startPage, endPage;
        startPage = (addr - cfg->startAddr) >> 11;
        endPage = ((addr - cfg->startAddr) + dataSz - 1) >> 11;

        /* Enable page erase mode */
        whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_PER_Msk,
                        whal_SetBits(FLASH_CR_PER_Msk, FLASH_CR_PER_Pos, 1));

        /* Erase each page in the range */
        for (size_t page = startPage; page <= endPage; ++page) {
            /* Select page number */
            whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_PNB_Msk,
                            whal_SetBits(FLASH_CR_PNB_Msk, FLASH_CR_PNB_Pos, page));

            /* Start erase operation */
            whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_STRT_Msk,
                            whal_SetBits(FLASH_CR_STRT_Msk, FLASH_CR_STRT_Pos, 1));

            /* Wait for erase to complete */
            err = whal_Reg_ReadPoll(base, FLASH_SR_REG,
                                    FLASH_SR_CFGBSY_Msk, 0, cfg->timeout);
            if (err)
                goto cleanup;
        }

        /* Disable page erase mode */
        whal_Reg_Update(base, FLASH_CR_REG, FLASH_CR_PER_Msk,
                        whal_SetBits(FLASH_CR_PER_Msk, FLASH_CR_PER_Pos, 0));
    }

cleanup:
    /* Clear programming and erase mode bits */
    whal_Reg_Update(base, FLASH_CR_REG,
                    FLASH_CR_PG_Msk | FLASH_CR_PER_Msk, 0);

    return err;
}

whal_Error whal_Stm32c0_Flash_Write(whal_Flash *flashDev, size_t addr, const void *data,
                                size_t dataSz)
{
    return whal_Stm32c0_Flash_WriteOrErase(flashDev, addr, (const uint8_t *)data, dataSz, 1);
}

whal_Error whal_Stm32c0_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                size_t dataSz)
{
    return whal_Stm32c0_Flash_WriteOrErase(flashDev, addr, NULL, dataSz, 0);
}

whal_Error whal_Stm32c0_Flash_Ext_SetLatency(whal_Flash *flashDev, enum whal_Stm32c0_Flash_Latency latency)
{
    size_t base = whal_Stm32c0_Flash_Dev.base;
    (void)flashDev;

    whal_Reg_Update(base, FLASH_ACR_REG, FLASH_ACR_LATENCY_Msk, latency);
    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32C0_FLASH_DIRECT_API_MAPPING
const whal_FlashDriver whal_Stm32c0_Flash_Driver = {
    .Init = whal_Stm32c0_Flash_Init,
    .Deinit = whal_Stm32c0_Flash_Deinit,
    .Lock = whal_Stm32c0_Flash_Lock,
    .Unlock = whal_Stm32c0_Flash_Unlock,
    .Read = whal_Stm32c0_Flash_Read,
    .Write = whal_Stm32c0_Flash_Write,
    .Erase = whal_Stm32c0_Flash_Erase,
};
#endif /* !WHAL_CFG_STM32C0_FLASH_DIRECT_API_MAPPING */
