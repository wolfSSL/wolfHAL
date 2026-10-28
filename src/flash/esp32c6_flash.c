/* esp32c6_flash.c
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

#include "wolfHAL_board.h"  /* provides WHAL_CFG_ESP32C6_FLASH_DEV initializer */
#include <wolfHAL/flash/esp32c6_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>

const whal_Flash whal_Esp32c6_Flash_Dev = WHAL_CFG_ESP32C6_FLASH_DEV;

/* SPI1 memory-SPI controller registers. Setting a CMD bit sends that flash
 * command and the bit clears once it has been sent; the flash itself may
 * still be busy, which RDSR's WIP bit reports. ADDR holds the 24-bit flash
 * offset and, for READ and PP, the byte count in bits 31:24. Data moves
 * through the 64-byte W0-W15 buffer. */
#define SPI_MEM_CMD_REG                 0x0000
#define SPI_MEM_FLASH_SE_Msk            (1UL << 24)
#define SPI_MEM_FLASH_PP_Msk            (1UL << 25)
#define SPI_MEM_FLASH_RDSR_Msk          (1UL << 27)
#define SPI_MEM_FLASH_WREN_Msk          (1UL << 30)
#define SPI_MEM_FLASH_READ_Msk          (1UL << 31)

#define SPI_MEM_ADDR_REG                0x0004
#define SPI_MEM_ADDR_LEN_Pos            24

#define SPI_MEM_RD_STATUS_REG           0x002C
#define FLASH_SR_WIP_Msk                (1UL << 0)

#define SPI_MEM_W0_REG                  0x0058

#define FLASH_BUF_SZ                    64
#define FLASH_PAGE_SZ                   256
#define FLASH_SECTOR_SZ                 4096

#define MSTATUS_MIE_Msk                 (1UL << 3)

#define RAMFUNC __attribute__((section(".ramfunc"), noinline))

#ifdef WHAL_CFG_ESP32C6_FLASH_DIRECT_API_MAPPING
#define whal_Esp32c6_Flash_Init   whal_Flash_Init
#define whal_Esp32c6_Flash_Deinit whal_Flash_Deinit
#define whal_Esp32c6_Flash_Lock   whal_Flash_Lock
#define whal_Esp32c6_Flash_Unlock whal_Flash_Unlock
#define whal_Esp32c6_Flash_Read   whal_Flash_Read
#define whal_Esp32c6_Flash_Write  whal_Flash_Write
#define whal_Esp32c6_Flash_Erase  whal_Flash_Erase
#endif /* WHAL_CFG_ESP32C6_FLASH_DIRECT_API_MAPPING */

/* Read up to FLASH_BUF_SZ bytes at flash offset off. */
static RAMFUNC void Esp32c6_Flash_RamRead(size_t base, size_t off,
                                          uint8_t *dst, size_t len)
{
    size_t mstatus;

    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(mstatus) : "r"(MSTATUS_MIE_Msk) : "memory");

    whal_Reg_Write(base, SPI_MEM_ADDR_REG, off | (len << SPI_MEM_ADDR_LEN_Pos));
    whal_Reg_Write(base, SPI_MEM_CMD_REG, SPI_MEM_FLASH_READ_Msk);
    while (whal_Reg_Read(base, SPI_MEM_CMD_REG));

    for (size_t i = 0; i < len; i++) {
        size_t word = whal_Reg_Read(base, SPI_MEM_W0_REG + (i & ~3UL));
        dst[i] = (uint8_t)(word >> (8 * (i & 3)));
    }

    __asm__ volatile("csrs mstatus, %0"
                     :: "r"(mstatus & MSTATUS_MIE_Msk) : "memory");
}

/* Program up to FLASH_BUF_SZ bytes at flash offset off, within one page. */
static RAMFUNC void Esp32c6_Flash_RamProgram(size_t base, size_t off,
                                             const uint8_t *src, size_t len)
{
    size_t mstatus;

    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(mstatus) : "r"(MSTATUS_MIE_Msk) : "memory");

    for (size_t i = 0; i < len; i += 4) {
        size_t word = 0;
        for (size_t j = 0; j < 4 && i + j < len; j++) {
            word |= (size_t)src[i + j] << (8 * j);
        }
        whal_Reg_Write(base, SPI_MEM_W0_REG + i, word);
    }

    whal_Reg_Write(base, SPI_MEM_CMD_REG, SPI_MEM_FLASH_WREN_Msk);
    while (whal_Reg_Read(base, SPI_MEM_CMD_REG));

    whal_Reg_Write(base, SPI_MEM_ADDR_REG, off | (len << SPI_MEM_ADDR_LEN_Pos));
    whal_Reg_Write(base, SPI_MEM_CMD_REG, SPI_MEM_FLASH_PP_Msk);
    while (whal_Reg_Read(base, SPI_MEM_CMD_REG));

    do {
        whal_Reg_Write(base, SPI_MEM_CMD_REG, SPI_MEM_FLASH_RDSR_Msk);
        while (whal_Reg_Read(base, SPI_MEM_CMD_REG));
    } while (whal_Reg_Read(base, SPI_MEM_RD_STATUS_REG) & FLASH_SR_WIP_Msk);

    __asm__ volatile("csrs mstatus, %0"
                     :: "r"(mstatus & MSTATUS_MIE_Msk) : "memory");
}

/* Erase the 4 KB sector at flash offset off. */
static RAMFUNC void Esp32c6_Flash_RamEraseSector(size_t base, size_t off)
{
    size_t mstatus;

    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(mstatus) : "r"(MSTATUS_MIE_Msk) : "memory");

    whal_Reg_Write(base, SPI_MEM_CMD_REG, SPI_MEM_FLASH_WREN_Msk);
    while (whal_Reg_Read(base, SPI_MEM_CMD_REG));

    whal_Reg_Write(base, SPI_MEM_ADDR_REG, off);
    whal_Reg_Write(base, SPI_MEM_CMD_REG, SPI_MEM_FLASH_SE_Msk);
    while (whal_Reg_Read(base, SPI_MEM_CMD_REG));

    do {
        whal_Reg_Write(base, SPI_MEM_CMD_REG, SPI_MEM_FLASH_RDSR_Msk);
        while (whal_Reg_Read(base, SPI_MEM_CMD_REG));
    } while (whal_Reg_Read(base, SPI_MEM_RD_STATUS_REG) & FLASH_SR_WIP_Msk);

    __asm__ volatile("csrs mstatus, %0"
                     :: "r"(mstatus & MSTATUS_MIE_Msk) : "memory");
}

whal_Error whal_Esp32c6_Flash_Init(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Flash_Deinit(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Flash_Lock(whal_Flash *flashDev, size_t addr, size_t len)
{
    (void)flashDev;
    (void)addr;
    (void)len;
    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Flash_Unlock(whal_Flash *flashDev, size_t addr, size_t len)
{
    (void)flashDev;
    (void)addr;
    (void)len;
    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Flash_Read(whal_Flash *flashDev, size_t addr, void *data,
                                   size_t dataSz)
{
    const whal_Esp32c6_Flash_Cfg *cfg =
        (const whal_Esp32c6_Flash_Cfg *)whal_Esp32c6_Flash_Dev.cfg;
    size_t base = whal_Esp32c6_Flash_Dev.base;
    uint8_t *buf = data;
    size_t off;
    (void)flashDev;

    if (!data) {
        return WHAL_EINVAL;
    }

    if (dataSz == 0) {
        return WHAL_SUCCESS;
    }

    if (addr < cfg->startAddr || dataSz > cfg->size ||
        addr - cfg->startAddr > cfg->size - dataSz) {
        return WHAL_EINVAL;
    }

    off = addr - cfg->startAddr;
    while (dataSz) {
        size_t len = dataSz < FLASH_BUF_SZ ? dataSz : FLASH_BUF_SZ;

        Esp32c6_Flash_RamRead(base, off, buf, len);
        off += len;
        buf += len;
        dataSz -= len;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Flash_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz)
{
    const whal_Esp32c6_Flash_Cfg *cfg =
        (const whal_Esp32c6_Flash_Cfg *)whal_Esp32c6_Flash_Dev.cfg;
    size_t base = whal_Esp32c6_Flash_Dev.base;
    const uint8_t *buf = data;
    size_t off;
    (void)flashDev;

    if (!data) {
        return WHAL_EINVAL;
    }

    if (dataSz == 0) {
        return WHAL_SUCCESS;
    }

    if (addr < cfg->startAddr || dataSz > cfg->size ||
        addr - cfg->startAddr > cfg->size - dataSz) {
        return WHAL_EINVAL;
    }

    off = addr - cfg->startAddr;
    while (dataSz) {
        size_t pageLeft = FLASH_PAGE_SZ - (off & (FLASH_PAGE_SZ - 1));
        size_t len = dataSz < FLASH_BUF_SZ ? dataSz : FLASH_BUF_SZ;

        if (len > pageLeft) {
            len = pageLeft;
        }

        Esp32c6_Flash_RamProgram(base, off, buf, len);
        off += len;
        buf += len;
        dataSz -= len;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Flash_Erase(whal_Flash *flashDev, size_t addr, size_t dataSz)
{
    const whal_Esp32c6_Flash_Cfg *cfg =
        (const whal_Esp32c6_Flash_Cfg *)whal_Esp32c6_Flash_Dev.cfg;
    size_t base = whal_Esp32c6_Flash_Dev.base;
    size_t first;
    size_t last;
    (void)flashDev;

    if (dataSz == 0) {
        return WHAL_SUCCESS;
    }

    if (addr < cfg->startAddr || dataSz > cfg->size ||
        addr - cfg->startAddr > cfg->size - dataSz) {
        return WHAL_EINVAL;
    }

    first = (addr - cfg->startAddr) & ~(FLASH_SECTOR_SZ - 1);
    last = (addr - cfg->startAddr + dataSz - 1) & ~(FLASH_SECTOR_SZ - 1);
    for (size_t off = first; off <= last; off += FLASH_SECTOR_SZ) {
        Esp32c6_Flash_RamEraseSector(base, off);
    }

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_ESP32C6_FLASH_DIRECT_API_MAPPING
const whal_FlashDriver whal_Esp32c6_Flash_Driver = {
    .Init = whal_Esp32c6_Flash_Init,
    .Deinit = whal_Esp32c6_Flash_Deinit,
    .Lock = whal_Esp32c6_Flash_Lock,
    .Unlock = whal_Esp32c6_Flash_Unlock,
    .Read = whal_Esp32c6_Flash_Read,
    .Write = whal_Esp32c6_Flash_Write,
    .Erase = whal_Esp32c6_Flash_Erase,
};
#endif /* !WHAL_CFG_ESP32C6_FLASH_DIRECT_API_MAPPING */
