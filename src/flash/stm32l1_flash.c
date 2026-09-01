/* stm32l1_flash.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32L1_FLASH_DEV initializer */
#include <wolfHAL/flash/stm32l1_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

const whal_Flash whal_Stm32l1_Flash_Dev = WHAL_CFG_STM32L1_FLASH_DEV;

/*
 * STM32L1 Flash Register Definitions (RM0038 Section 3.9)
 *
 * The STM32L1 flash uses a PECR-based access model that differs from
 * other STM32 families. All program/erase operations are controlled
 * through FLASH_PECR bits, and unlocking requires a two-stage key
 * sequence (PEKEYR then PRGKEYR).
 */

#define FLASH_ACR_REG     0x00
#define FLASH_ACR_LATENCY_Pos 0
#define FLASH_ACR_LATENCY_Msk (1UL << FLASH_ACR_LATENCY_Pos)
#define FLASH_ACR_PRFTEN_Pos  1
#define FLASH_ACR_PRFTEN_Msk  (1UL << FLASH_ACR_PRFTEN_Pos)
#define FLASH_ACR_ACC64_Pos   2
#define FLASH_ACR_ACC64_Msk   (1UL << FLASH_ACR_ACC64_Pos)

#define FLASH_BASE_ADDR 0x40023C00

#define FLASH_PECR_REG    0x04
#define FLASH_PECR_PELOCK_Pos  0
#define FLASH_PECR_PELOCK_Msk  (1UL << FLASH_PECR_PELOCK_Pos)
#define FLASH_PECR_PRGLOCK_Pos 1
#define FLASH_PECR_PRGLOCK_Msk (1UL << FLASH_PECR_PRGLOCK_Pos)
#define FLASH_PECR_PROG_Pos    3
#define FLASH_PECR_PROG_Msk    (1UL << FLASH_PECR_PROG_Pos)
#define FLASH_PECR_ERASE_Pos   9
#define FLASH_PECR_ERASE_Msk   (1UL << FLASH_PECR_ERASE_Pos)

#define FLASH_PEKEYR_REG  0x0C
#define FLASH_PRGKEYR_REG 0x10

#define FLASH_SR_REG      0x18
#define FLASH_SR_BSY_Pos  0
#define FLASH_SR_BSY_Msk  (1UL << FLASH_SR_BSY_Pos)
#define FLASH_SR_EOP_Pos  1
#define FLASH_SR_EOP_Msk  (1UL << FLASH_SR_EOP_Pos)
#define FLASH_SR_WRPERR_Pos 8
#define FLASH_SR_WRPERR_Msk (1UL << FLASH_SR_WRPERR_Pos)
#define FLASH_SR_PGAERR_Pos 9
#define FLASH_SR_PGAERR_Msk (1UL << FLASH_SR_PGAERR_Pos)
#define FLASH_SR_SIZERR_Pos 10
#define FLASH_SR_SIZERR_Msk (1UL << FLASH_SR_SIZERR_Pos)

#define FLASH_SR_ALL_ERR (FLASH_SR_WRPERR_Msk | FLASH_SR_PGAERR_Msk | \
                          FLASH_SR_SIZERR_Msk)

/* Unlock keys */
#define PEKEY1 0x89ABCDEF
#define PEKEY2 0x02030405
#define PRGKEY1 0x8C9DAEBF
#define PRGKEY2 0x13141516

/* Page size: 256 bytes */
#define PAGE_SIZE 256
#define PAGE_SHIFT 8

#ifdef WHAL_CFG_STM32L1_FLASH_DIRECT_API_MAPPING
#define whal_Stm32l1_Flash_Init   whal_Flash_Init
#define whal_Stm32l1_Flash_Deinit whal_Flash_Deinit
#define whal_Stm32l1_Flash_Lock   whal_Flash_Lock
#define whal_Stm32l1_Flash_Unlock whal_Flash_Unlock
#define whal_Stm32l1_Flash_Read   whal_Flash_Read
#define whal_Stm32l1_Flash_Write  whal_Flash_Write
#define whal_Stm32l1_Flash_Erase  whal_Flash_Erase
#endif

/*
 * Unlock FLASH_PECR by writing the PEKEY sequence to FLASH_PEKEYR.
 * This clears PELOCK if the keys are correct.
 */
static void UnlockPecr(size_t base)
{
    size_t pelock;
    whal_Reg_Get(base, FLASH_PECR_REG, FLASH_PECR_PELOCK_Msk,
                 FLASH_PECR_PELOCK_Pos, &pelock);
    if (!pelock)
        return;

    whal_Reg_Write(base, FLASH_PEKEYR_REG, PEKEY1);
    whal_Reg_Write(base, FLASH_PEKEYR_REG, PEKEY2);
}

/*
 * Unlock program memory by writing the PRGKEY sequence to FLASH_PRGKEYR.
 * PECR must be unlocked first.
 */
static void UnlockProgram(size_t base)
{
    size_t prglock;
    whal_Reg_Get(base, FLASH_PECR_REG, FLASH_PECR_PRGLOCK_Msk,
                 FLASH_PECR_PRGLOCK_Pos, &prglock);
    if (!prglock)
        return;

    whal_Reg_Write(base, FLASH_PRGKEYR_REG, PRGKEY1);
    whal_Reg_Write(base, FLASH_PRGKEYR_REG, PRGKEY2);
}

whal_Error whal_Stm32l1_Flash_Init(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32l1_Flash_Deinit(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32l1_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len)
{
    size_t base = whal_Stm32l1_Flash_Dev.base;
    (void)flashDev;
    (void)addr;
    (void)len;

    /* Set PELOCK to lock everything */
    whal_Reg_Update(base, FLASH_PECR_REG,
                    FLASH_PECR_PELOCK_Msk,
                    whal_SetBits(FLASH_PECR_PELOCK_Msk,
                                 FLASH_PECR_PELOCK_Pos, 1));

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32l1_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len)
{
    size_t base = whal_Stm32l1_Flash_Dev.base;
    (void)flashDev;
    (void)addr;
    (void)len;

    /* Unlock PECR then program memory */
    UnlockPecr(base);
    UnlockProgram(base);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32l1_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                                   size_t dataSz)
{
    const whal_Stm32l1_Flash_Cfg *cfg =
        (const whal_Stm32l1_Flash_Cfg *)whal_Stm32l1_Flash_Dev.cfg;
    uint8_t *dataBuf = (uint8_t *)data;
    (void)flashDev;

    if (!data)
        return WHAL_EINVAL;

    if (dataSz == 0)
        return WHAL_SUCCESS;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Flash is memory-mapped, direct read */
    uint8_t *flashAddr = (uint8_t *)addr;
    for (size_t i = 0; i < dataSz; ++i)
        dataBuf[i] = flashAddr[i];

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32l1_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz)
{
    const whal_Stm32l1_Flash_Cfg *cfg =
        (const whal_Stm32l1_Flash_Cfg *)whal_Stm32l1_Flash_Dev.cfg;
    size_t base = whal_Stm32l1_Flash_Dev.base;
    const uint8_t *buf = (const uint8_t *)data;
    whal_Error err = WHAL_SUCCESS;
    (void)flashDev;

    if (!data)
        return WHAL_EINVAL;

    if (dataSz == 0)
        return WHAL_SUCCESS;

    /* Word-aligned writes only */
    if ((addr & 0x3) || (dataSz & 0x3))
        return WHAL_EINVAL;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Clear error flags */
    whal_Reg_Write(base, FLASH_SR_REG, FLASH_SR_ALL_ERR);

    /*
     * Fast Word Write: PECR and program memory must already be unlocked.
     * Simply write a 32-bit word to the flash address; the hardware
     * performs the programming automatically.
     */
    for (size_t i = 0; i < dataSz; i += 4) {
        volatile uint32_t *flashAddr = (volatile uint32_t *)(addr + i);
        uint32_t word = (uint32_t)buf[i]
                      | ((uint32_t)buf[i + 1] << 8)
                      | ((uint32_t)buf[i + 2] << 16)
                      | ((uint32_t)buf[i + 3] << 24);

        *flashAddr = word;

        err = whal_Reg_ReadPoll(base, FLASH_SR_REG, FLASH_SR_BSY_Msk, 0,
                                cfg->timeout);
        if (err)
            return err;

        if (whal_Reg_Read(base, FLASH_SR_REG) & FLASH_SR_ALL_ERR)
            return WHAL_EHARDWARE;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32l1_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                    size_t dataSz)
{
    const whal_Stm32l1_Flash_Cfg *cfg =
        (const whal_Stm32l1_Flash_Cfg *)whal_Stm32l1_Flash_Dev.cfg;
    size_t base = whal_Stm32l1_Flash_Dev.base;
    whal_Error err = WHAL_SUCCESS;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_SUCCESS;

    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    err = whal_Reg_ReadPoll(base, FLASH_SR_REG, FLASH_SR_BSY_Msk, 0,
                            cfg->timeout);
    if (err)
        return err;

    whal_Reg_Write(base, FLASH_SR_REG, FLASH_SR_ALL_ERR);

    size_t startPage = (addr - cfg->startAddr) >> PAGE_SHIFT;
    size_t endPage = ((addr - cfg->startAddr) + dataSz - 1) >> PAGE_SHIFT;

    whal_Reg_Update(base, FLASH_PECR_REG, FLASH_PECR_ERASE_Msk,
                    whal_SetBits(FLASH_PECR_ERASE_Msk,
                                 FLASH_PECR_ERASE_Pos, 1));
    whal_Reg_Update(base, FLASH_PECR_REG, FLASH_PECR_PROG_Msk,
                    whal_SetBits(FLASH_PECR_PROG_Msk,
                                 FLASH_PECR_PROG_Pos, 1));

    for (size_t page = startPage; page <= endPage; ++page) {
        volatile uint32_t *pageAddr =
            (volatile uint32_t *)(cfg->startAddr + (page << PAGE_SHIFT));
        *pageAddr = 0x00000000;

        err = whal_Reg_ReadPoll(base, FLASH_SR_REG, FLASH_SR_BSY_Msk, 0,
                                cfg->timeout);
        if (err)
            goto cleanup;

        if (whal_Reg_Read(base, FLASH_SR_REG) & FLASH_SR_ALL_ERR) {
            err = WHAL_EHARDWARE;
            goto cleanup;
        }
    }

cleanup:
    whal_Reg_Update(base, FLASH_PECR_REG,
                    FLASH_PECR_ERASE_Msk | FLASH_PECR_PROG_Msk, 0);

    return err;
}

whal_Error whal_Stm32l1_Flash_Ext_SetLatency(whal_Stm32l1_Flash_Latency latency)
{
    size_t val;

    whal_Reg_Update(FLASH_BASE_ADDR, FLASH_ACR_REG, FLASH_ACR_ACC64_Msk,
                    whal_SetBits(FLASH_ACR_ACC64_Msk,
                                 FLASH_ACR_ACC64_Pos, 1));
    do {
        whal_Reg_Get(FLASH_BASE_ADDR, FLASH_ACR_REG,
                     FLASH_ACR_ACC64_Msk, FLASH_ACR_ACC64_Pos, &val);
    } while (!val);

    whal_Reg_Update(FLASH_BASE_ADDR, FLASH_ACR_REG, FLASH_ACR_PRFTEN_Msk,
                    whal_SetBits(FLASH_ACR_PRFTEN_Msk,
                                 FLASH_ACR_PRFTEN_Pos, 1));

    whal_Reg_Update(FLASH_BASE_ADDR, FLASH_ACR_REG, FLASH_ACR_LATENCY_Msk,
                    whal_SetBits(FLASH_ACR_LATENCY_Msk,
                                 FLASH_ACR_LATENCY_Pos, latency));
    do {
        whal_Reg_Get(FLASH_BASE_ADDR, FLASH_ACR_REG,
                     FLASH_ACR_LATENCY_Msk, FLASH_ACR_LATENCY_Pos, &val);
    } while (val != (size_t)latency);

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32L1_FLASH_DIRECT_API_MAPPING
const whal_FlashDriver whal_Stm32l1_Flash_Driver = {
    .Init = whal_Stm32l1_Flash_Init,
    .Deinit = whal_Stm32l1_Flash_Deinit,
    .Lock = whal_Stm32l1_Flash_Lock,
    .Unlock = whal_Stm32l1_Flash_Unlock,
    .Read = whal_Stm32l1_Flash_Read,
    .Write = whal_Stm32l1_Flash_Write,
    .Erase = whal_Stm32l1_Flash_Erase,
};
#endif /* !WHAL_CFG_STM32L1_FLASH_DIRECT_API_MAPPING */
