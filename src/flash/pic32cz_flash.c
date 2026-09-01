/* pic32cz_flash.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_PIC32CZ_FLASH_DEV initializer */
#include <wolfHAL/flash/pic32cz_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

const whal_Flash whal_Pic32cz_Flash_Dev = WHAL_CFG_PIC32CZ_FLASH_DEV;

/*
 * PIC32CZ FCW (Flash Controller Write) Register Definitions
 *
 * The PIC32CZ uses FCW for flash write/erase operations. Each operation
 * requires writing an unlock key to FCW_KEY before triggering via FCW_CTRLA.
 */

/* Register offsets */
#define FCW_CTRLA_REG       0x00
#define FCW_MUTEX_REG       0x08
#define FCW_INTFLAG_REG     0x14
#define FCW_STATUS_REG      0x18
#define FCW_KEY_REG         0x1C
#define FCW_ADDR_REG        0x20
#define FCW_SRCADDR_REG        0x24
#define FCW_DATA_REG(n)     (0x28 + ((n) * 4))

/* CTRLA: NVM operation command and pre-program bit */
#define FCW_CTRLA_NVMOP_Pos               0
#define FCW_CTRLA_NVMOP_Msk               (WHAL_BITMASK(4) << FCW_CTRLA_NVMOP_Pos)
#define FCW_CTRLA_NVMOP_SINGLE_DWORD       0x1
#define FCW_CTRLA_NVMOP_QUAD_DWORD         0x2
#define FCW_CTRLA_NVMOP_PAGE_ERASE         0x4

#define FCW_CTRLA_PREPG_Pos                     7
#define FCW_CTRLA_PREPG_Msk                     (1UL << FCW_CTRLA_PREPG_Pos)

/* MUTEX: NVM locking and ownership values */
#define FCW_MUTEX_LOCK_Pos 0
#define FCW_MUTEX_LOCK_Msk (1UL << FCW_MUTEX_LOCK_Pos)

#define FCW_MUTEX_OWNER_Pos 1
#define FCW_MUTEX_OWNER_Msk (WHAL_BITMASK(2) << FCW_MUTEX_OWNER_Pos)

/* INTFLAG: operation completion and error flags (write-1-to-clear) */
#define FCW_INTFLAG_DONE_Pos        0
#define FCW_INTFLAG_DONE_Msk        (1UL << FCW_INTFLAG_DONE_Pos)

#define FCW_INTFLAG_KEYERR_Pos      1
#define FCW_INTFLAG_KEYERR_Msk      (1UL << FCW_INTFLAG_KEYERR_Pos)

#define FCW_INTFLAG_CFGERR_Pos      2
#define FCW_INTFLAG_CFGERR_Msk      (1UL << FCW_INTFLAG_CFGERR_Pos)

#define FCW_INTFLAG_FIFOERR_Pos     3
#define FCW_INTFLAG_FIFOERR_Msk     (1UL << FCW_INTFLAG_FIFOERR_Pos)

#define FCW_INTFLAG_BUSERR_Pos      4
#define FCW_INTFLAG_BUSERR_Msk      (1UL << FCW_INTFLAG_BUSERR_Pos)

#define FCW_INTFLAG_WPERR_Pos       5
#define FCW_INTFLAG_WPERR_Msk       (1UL << FCW_INTFLAG_WPERR_Pos)

#define FCW_INTFLAG_OPERR_Pos       6
#define FCW_INTFLAG_OPERR_Msk       (1UL << FCW_INTFLAG_OPERR_Pos)

#define FCW_INTFLAG_SECERR_Pos      7
#define FCW_INTFLAG_SECERR_Msk      (1UL << FCW_INTFLAG_SECERR_Pos)

#define FCW_INTFLAG_HTDPGM_Pos      8
#define FCW_INTFLAG_HTDPGM_Msk      (1UL << FCW_INTFLAG_HTDPGM_Pos)

#define FCW_INTFLAG_BORERR_Pos      12
#define FCW_INTFLAG_BORERR_Msk      (1UL << FCW_INTFLAG_BORERR_Pos)

#define FCW_INTFLAG_WRERR_Pos       13
#define FCW_INTFLAG_WRERR_Msk       (1UL << FCW_INTFLAG_WRERR_Pos)

#define FCW_INTFLAG_ALL_ERR     (FCW_INTFLAG_KEYERR_Msk | FCW_INTFLAG_CFGERR_Msk | \
                                 FCW_INTFLAG_FIFOERR_Msk | FCW_INTFLAG_BUSERR_Msk | \
                                 FCW_INTFLAG_WPERR_Msk | FCW_INTFLAG_OPERR_Msk | \
                                 FCW_INTFLAG_SECERR_Msk | FCW_INTFLAG_HTDPGM_Msk | \
                                 FCW_INTFLAG_BORERR_Msk | FCW_INTFLAG_WRERR_Msk)

#define FCW_INTFLAG_ALL         (FCW_INTFLAG_DONE_Msk | FCW_INTFLAG_ALL_ERR)

/* STATUS: busy flag */
#define FCW_STATUS_BUSY_Pos         0
#define FCW_STATUS_BUSY_Msk         (1UL << FCW_STATUS_BUSY_Pos)

/* KEY: unlock value for write/erase operations */
#define FCW_UNLOCK_WRKEY        0x91C32C01

/* Flash geometry */
#define FCW_PAGE_SIZE           4096
#define FCW_DWORD_SIZE          8
#define FCW_QDWORD_SIZE         32

#ifdef WHAL_CFG_PIC32CZ_FLASH_DIRECT_API_MAPPING
#define whal_Pic32cz_Flash_Init   whal_Flash_Init
#define whal_Pic32cz_Flash_Deinit whal_Flash_Deinit
#define whal_Pic32cz_Flash_Lock   whal_Flash_Lock
#define whal_Pic32cz_Flash_Unlock whal_Flash_Unlock
#define whal_Pic32cz_Flash_Read   whal_Flash_Read
#define whal_Pic32cz_Flash_Write  whal_Flash_Write
#define whal_Pic32cz_Flash_Erase  whal_Flash_Erase
#endif /* WHAL_CFG_PIC32CZ_FLASH_DIRECT_API_MAPPING */

static whal_Error whal_Pic32cz_Flash_MutexLock(size_t base,
                                              whal_Timeout *timeout)
{
    WHAL_TIMEOUT_START(timeout);
    while (whal_Reg_Read(base, FCW_MUTEX_REG) & FCW_MUTEX_LOCK_Msk) {
        if (WHAL_TIMEOUT_EXPIRED(timeout))
            return WHAL_ETIMEOUT;
    }

    whal_Reg_Update(base, FCW_MUTEX_REG, FCW_MUTEX_LOCK_Msk | FCW_MUTEX_OWNER_Msk,
                    whal_SetBits(FCW_MUTEX_LOCK_Msk, FCW_MUTEX_LOCK_Pos, 1) |
                    whal_SetBits(FCW_MUTEX_OWNER_Msk, FCW_MUTEX_OWNER_Pos, 1));

    return WHAL_SUCCESS;
}

static void whal_Pic32cz_Flash_MutexUnlock(size_t base)
{
    whal_Reg_Update(base, FCW_MUTEX_REG, FCW_MUTEX_LOCK_Msk | FCW_MUTEX_OWNER_Msk,
                    whal_SetBits(FCW_MUTEX_LOCK_Msk, FCW_MUTEX_LOCK_Pos, 0) |
                    whal_SetBits(FCW_MUTEX_OWNER_Msk, FCW_MUTEX_OWNER_Pos, 1));
}

static whal_Error whal_Pic32cz_Flash_WaitBusy(size_t base,
                                              whal_Timeout *timeout)
{
    WHAL_TIMEOUT_START(timeout);
    while (whal_Reg_Read(base, FCW_STATUS_REG) & FCW_STATUS_BUSY_Msk) {
        if (WHAL_TIMEOUT_EXPIRED(timeout))
            return WHAL_ETIMEOUT;
    }

    return WHAL_SUCCESS;
}

/*
 * Execute an FCW command: unlock, trigger, wait, and check for errors.
 * Caller must set up FCW_ADDR and FCW_DATA registers before calling.
 */
static whal_Error whal_Pic32cz_Flash_ExecCmd(size_t base, size_t cmd,
                                            whal_Timeout *timeout)
{
    whal_Error err;
    size_t errFlags;

    /* Write unlock key */
    whal_Reg_Update(base, FCW_KEY_REG, 0xFFFFFFFF, FCW_UNLOCK_WRKEY);

    /* Trigger operation with pre-program enabled */
    whal_Reg_Update(base, FCW_CTRLA_REG,
                    FCW_CTRLA_NVMOP_Msk | FCW_CTRLA_PREPG_Msk,
                    whal_SetBits(FCW_CTRLA_NVMOP_Msk, FCW_CTRLA_NVMOP_Pos, cmd) | FCW_CTRLA_PREPG_Msk);

    /* Wait for completion */
    err = whal_Pic32cz_Flash_WaitBusy(base, timeout);
    if (err)
        return err;

    /* Check for errors */
    whal_Reg_Get(base, FCW_INTFLAG_REG, FCW_INTFLAG_ALL_ERR, 0, &errFlags);

    /* Clear DONE flag */
    whal_Reg_Update(base, FCW_INTFLAG_REG, FCW_INTFLAG_DONE_Msk,
                    FCW_INTFLAG_DONE_Msk);

    if (errFlags) {
        /* Clear error flags */
        whal_Reg_Update(base, FCW_INTFLAG_REG, FCW_INTFLAG_ALL_ERR,
                        FCW_INTFLAG_ALL_ERR);
        return WHAL_EINVAL;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Flash_Init(whal_Flash *flashDev)
{
    size_t base = whal_Pic32cz_Flash_Dev.base;
    (void)flashDev;

    whal_Pic32cz_Flash_MutexUnlock(base);
    whal_Reg_Update(base, FCW_KEY_REG, 0xFFFFFFFF, 0);
    whal_Reg_Update(base, FCW_ADDR_REG, 0xFFFFFFFF, 0);
    whal_Reg_Update(base, FCW_SRCADDR_REG, 0xFFFFFFFF, 0);

    /* Clear all pending interrupt flags */
    whal_Reg_Update(base, FCW_INTFLAG_REG, FCW_INTFLAG_ALL,
                    FCW_INTFLAG_ALL);

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Flash_Deinit(whal_Flash *flashDev)
{
    size_t base = whal_Pic32cz_Flash_Dev.base;
    (void)flashDev;

    whal_Pic32cz_Flash_MutexUnlock(base);
    whal_Reg_Update(base, FCW_KEY_REG, 0xFFFFFFFF, 0);
    whal_Reg_Update(base, FCW_ADDR_REG, 0xFFFFFFFF, 0);
    whal_Reg_Update(base, FCW_SRCADDR_REG, 0xFFFFFFFF, 0);

    /* Clear all pending interrupt flags */
    whal_Reg_Update(base, FCW_INTFLAG_REG, FCW_INTFLAG_ALL,
                    FCW_INTFLAG_ALL);

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len)
{
    /*
     * TODO: Implement using FCW_PWP[0..7] region write-protect registers.
     * Each FCW operation already requires the unlock key, providing
     * inherent per-operation protection.
     */
    (void)flashDev;
    (void)addr;
    (void)len;

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len)
{
    (void)flashDev;
    (void)addr;
    (void)len;

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                             size_t dataSz)
{
    const whal_Pic32cz_Flash_Cfg *cfg =
        (const whal_Pic32cz_Flash_Cfg *)whal_Pic32cz_Flash_Dev.cfg;
    size_t base = whal_Pic32cz_Flash_Dev.base;
    uint8_t *dataBuf = (uint8_t *)data;
    uint8_t *flashAddr = (uint8_t *)addr;
    whal_Error err;
    size_t i;
    (void)flashDev;

    if (!data) {
        return WHAL_EINVAL;
    }

    err = whal_Pic32cz_Flash_MutexLock(base, cfg->timeout);
    if (err)
        return err;

    /* Flash is memory-mapped; read directly */
    for (i = 0; i < dataSz; i++) {
        dataBuf[i] = flashAddr[i];
    }

    whal_Pic32cz_Flash_MutexUnlock(base);

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Flash_Write(whal_Flash *flashDev, size_t addr, const void *data,
                              size_t dataSz)
{
    const whal_Pic32cz_Flash_Cfg *cfg =
        (const whal_Pic32cz_Flash_Cfg *)whal_Pic32cz_Flash_Dev.cfg;
    size_t base = whal_Pic32cz_Flash_Dev.base;
    const uint8_t *dataBuf = (const uint8_t *)data;
    const uint32_t *src;
    whal_Error err;
    size_t offset = 0;
    (void)flashDev;

    if (!data) {
        return WHAL_EINVAL;
    }

    /* Require double-word alignment */
    if ((addr & 0x7) || (dataSz & 0x7)) {
        return WHAL_EINVAL;
    }

    src = (const uint32_t *)dataBuf;


    err = whal_Pic32cz_Flash_MutexLock(base, cfg->timeout);
    if (err)
        return err;

    while (offset < dataSz) {
        size_t curAddr = addr + offset;
        size_t remaining = dataSz - offset;

        err = whal_Pic32cz_Flash_WaitBusy(base, cfg->timeout);
        if (err) {
            whal_Pic32cz_Flash_MutexUnlock(base);
            return err;
        }

        if (!(curAddr & 0x1F) && remaining >= FCW_QDWORD_SIZE) {
            /* Quad double word write (32 bytes) */
            size_t j;
            for (j = 0; j < 8; j++) {
                whal_Reg_Update(base, FCW_DATA_REG(j),
                                0xFFFFFFFF, src[offset / 4 + j]);
            }
            whal_Reg_Update(base, FCW_ADDR_REG, 0xFFFFFFFF, curAddr);

            err = whal_Pic32cz_Flash_ExecCmd(base,
                                            FCW_CTRLA_NVMOP_QUAD_DWORD,
                                            cfg->timeout);
            if (err) {
                whal_Pic32cz_Flash_MutexUnlock(base);
                return err;
            }
            offset += FCW_QDWORD_SIZE;
        } else {
            /* Single double word write (8 bytes) */
            whal_Reg_Update(base, FCW_DATA_REG(0),
                            0xFFFFFFFF, src[offset / 4]);
            whal_Reg_Update(base, FCW_DATA_REG(1),
                            0xFFFFFFFF, src[offset / 4 + 1]);
            whal_Reg_Update(base, FCW_ADDR_REG, 0xFFFFFFFF, curAddr);

            err = whal_Pic32cz_Flash_ExecCmd(base,
                                            FCW_CTRLA_NVMOP_SINGLE_DWORD,
                                            cfg->timeout);
            if (err) {
                whal_Pic32cz_Flash_MutexUnlock(base);
                return err;
            }
            offset += FCW_DWORD_SIZE;
        }

    }

    whal_Pic32cz_Flash_MutexUnlock(base);

    return WHAL_SUCCESS;
}

whal_Error whal_Pic32cz_Flash_Erase(whal_Flash *flashDev, size_t addr, size_t dataSz)
{
    const whal_Pic32cz_Flash_Cfg *cfg =
        (const whal_Pic32cz_Flash_Cfg *)whal_Pic32cz_Flash_Dev.cfg;
    size_t base = whal_Pic32cz_Flash_Dev.base;
    whal_Error err;
    size_t pageAddr;
    size_t endAddr;
    (void)flashDev;

    /* Align down to page boundary */
    pageAddr = addr & ~(FCW_PAGE_SIZE - 1);
    endAddr = addr + dataSz;


    err = whal_Pic32cz_Flash_MutexLock(base, cfg->timeout);
    if (err)
        return err;

    while (pageAddr < endAddr) {
        err = whal_Pic32cz_Flash_WaitBusy(base, cfg->timeout);
        if (err) {
            whal_Pic32cz_Flash_MutexUnlock(base);
            return err;
        }

        whal_Reg_Update(base, FCW_ADDR_REG, 0xFFFFFFFF, pageAddr);

        err = whal_Pic32cz_Flash_ExecCmd(base, FCW_CTRLA_NVMOP_PAGE_ERASE,
                                        cfg->timeout);
        if (err) {
            whal_Pic32cz_Flash_MutexUnlock(base);
            return err;
        }

        pageAddr += FCW_PAGE_SIZE;
    }

    whal_Pic32cz_Flash_MutexUnlock(base);

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_PIC32CZ_FLASH_DIRECT_API_MAPPING
const whal_FlashDriver whal_Pic32cz_Flash_Driver = {
    .Init = whal_Pic32cz_Flash_Init,
    .Deinit = whal_Pic32cz_Flash_Deinit,
    .Lock = whal_Pic32cz_Flash_Lock,
    .Unlock = whal_Pic32cz_Flash_Unlock,
    .Read = whal_Pic32cz_Flash_Read,
    .Write = whal_Pic32cz_Flash_Write,
    .Erase = whal_Pic32cz_Flash_Erase,
};
#endif /* !WHAL_CFG_PIC32CZ_FLASH_DIRECT_API_MAPPING */
