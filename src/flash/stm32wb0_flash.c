/* stm32wb0_flash.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32WB0_FLASH_DEV initializer */
#include <wolfHAL/reg.h>
#include <wolfHAL/flash/stm32wb0_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/error.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>

const whal_Flash whal_Stm32wb0_Flash_Dev = WHAL_CFG_STM32WB0_FLASH_DEV;

/*
 * STM32WB0 Flash controller register layout (RM0529 section 9.4).
 *
 * The controller is command-driven: write an opcode to COMMAND, watch
 * IRQSTAT for CMDSTART then CMDDONE, then clear those flags by writing
 * 1 to the corresponding bit. ADDRESS is encoded in flash-internal word
 * coordinates (page+row+word), not bytes.
 */

#define FLASH_COMMAND_REG   0x00
#define FLASH_COMMAND_ERASE        0x11
#define FLASH_COMMAND_MASSERASE    0x22
#define FLASH_COMMAND_WRITE        0x33
#define FLASH_COMMAND_MASSREAD     0x55
#define FLASH_COMMAND_SLEEP        0xAA
#define FLASH_COMMAND_WAKEUP       0xBB
#define FLASH_COMMAND_BURSTWRITE   0xCC
#define FLASH_COMMAND_OTPWRITE     0xEE
#define FLASH_COMMAND_KEYWRITE     0xFF

#define FLASH_CONFIG_REG    0x04
#define FLASH_CONFIG_WAIT_STATES_Pos 4
#define FLASH_CONFIG_WAIT_STATES_Msk (WHAL_BITMASK(2) << FLASH_CONFIG_WAIT_STATES_Pos)

#define FLASH_IRQSTAT_REG   0x08
#define FLASH_IRQRAW_REG    0x10
#define FLASH_IRQ_CMDDONE_Pos  0
#define FLASH_IRQ_CMDDONE_Msk  (1UL << FLASH_IRQ_CMDDONE_Pos)
#define FLASH_IRQ_CMDSTART_Pos 1
#define FLASH_IRQ_CMDSTART_Msk (1UL << FLASH_IRQ_CMDSTART_Pos)
#define FLASH_IRQ_CMDERR_Pos   2
#define FLASH_IRQ_CMDERR_Msk   (1UL << FLASH_IRQ_CMDERR_Pos)
#define FLASH_IRQ_ILLCMD_Pos   3
#define FLASH_IRQ_ILLCMD_Msk   (1UL << FLASH_IRQ_ILLCMD_Pos)
#define FLASH_IRQ_READOK_Pos   4
#define FLASH_IRQ_READOK_Msk   (1UL << FLASH_IRQ_READOK_Pos)
#define FLASH_IRQ_ALL_ERR_Msk  (FLASH_IRQ_CMDERR_Msk | FLASH_IRQ_ILLCMD_Msk)

#define FLASH_ADDRESS_REG   0x18
#define FLASH_DATA0_REG     0x40
#define FLASH_DATA1_REG     0x44
#define FLASH_DATA2_REG     0x48
#define FLASH_DATA3_REG     0x4C

/* WB0 page size: 2 KB. ADDRESS encodes word index; one word = 4 bytes. */
#define FLASH_PAGE_SIZE     0x800u
#define FLASH_WORD_SIZE     4u

#ifdef WHAL_CFG_STM32WB0_FLASH_DIRECT_API_MAPPING
#define whal_Stm32wb0_Flash_Init   whal_Flash_Init
#define whal_Stm32wb0_Flash_Deinit whal_Flash_Deinit
#define whal_Stm32wb0_Flash_Lock   whal_Flash_Lock
#define whal_Stm32wb0_Flash_Unlock whal_Flash_Unlock
#define whal_Stm32wb0_Flash_Read   whal_Flash_Read
#define whal_Stm32wb0_Flash_Write  whal_Flash_Write
#define whal_Stm32wb0_Flash_Erase  whal_Flash_Erase
#endif /* WHAL_CFG_STM32WB0_FLASH_DIRECT_API_MAPPING */

/*
 * Run a single Flash controller command: poll CMDSTART, clear it, poll
 * CMDDONE, clear it, then sweep CMDERR/ILLCMD into an error code. All
 * polling loops are bounded by the configured whal_Timeout.
 */
static whal_Error whal_Stm32wb0_Flash_RunCommand(size_t base, uint32_t cmd,
                                                 whal_Timeout *timeout)
{
    whal_Error err;
    size_t irq;

    /* Clear any stale start/done/error flags before launching. */
    whal_Reg_Write(base, FLASH_IRQSTAT_REG,
                   FLASH_IRQ_CMDSTART_Msk | FLASH_IRQ_CMDDONE_Msk |
                       FLASH_IRQ_ALL_ERR_Msk);

    whal_Reg_Write(base, FLASH_COMMAND_REG, cmd);

    /* Wait for CMDSTART (controller has accepted the opcode). */
    WHAL_TIMEOUT_START(timeout);
    while (1) {
        if (WHAL_TIMEOUT_EXPIRED(timeout))
            return WHAL_ETIMEOUT;
        irq = whal_Reg_Read(base, FLASH_IRQRAW_REG);
        if (irq & FLASH_IRQ_ALL_ERR_Msk) {
            whal_Reg_Write(base, FLASH_IRQSTAT_REG, FLASH_IRQ_ALL_ERR_Msk);
            return WHAL_EHARDWARE;
        }
        if (irq & FLASH_IRQ_CMDSTART_Msk)
            break;
    }
    whal_Reg_Write(base, FLASH_IRQSTAT_REG, FLASH_IRQ_CMDSTART_Msk);

    /* Wait for CMDDONE. */
    WHAL_TIMEOUT_START(timeout);
    while (1) {
        if (WHAL_TIMEOUT_EXPIRED(timeout))
            return WHAL_ETIMEOUT;
        irq = whal_Reg_Read(base, FLASH_IRQRAW_REG);
        if (irq & FLASH_IRQ_ALL_ERR_Msk) {
            whal_Reg_Write(base, FLASH_IRQSTAT_REG, FLASH_IRQ_ALL_ERR_Msk);
            err = WHAL_EHARDWARE;
            /* Still clear CMDDONE if present so we don't leave it set. */
            whal_Reg_Write(base, FLASH_IRQSTAT_REG, FLASH_IRQ_CMDDONE_Msk);
            return err;
        }
        if (irq & FLASH_IRQ_CMDDONE_Msk)
            break;
    }
    whal_Reg_Write(base, FLASH_IRQSTAT_REG, FLASH_IRQ_CMDDONE_Msk);

    return WHAL_SUCCESS;
}

/*
 * Convert a byte address inside the main flash array into the
 * controller's word-index format (page<<9 | row<<6 | word).
 *
 * The hardware uses ADDRESS[15:0] = XADR[9:0] & YADR[5:0]:
 *   XADR[9:3] = page address
 *   XADR[2:0] = row address (8 rows per page)
 *   YADR[5:0] = word address (64 words per row)
 * Combined, the field is (byteOffset >> 2) where byteOffset is the
 * offset from cfg->startAddr. The shift drops the 2-bit byte index
 * since the controller addresses 32-bit words.
 */
static uint32_t whal_Stm32wb0_Flash_AddrToHw(const whal_Stm32wb0_Flash_Cfg *cfg,
                                             size_t addr)
{
    return (uint32_t)((addr - cfg->startAddr) >> 2);
}

whal_Error whal_Stm32wb0_Flash_Init(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb0_Flash_Deinit(whal_Flash *flashDev)
{
    (void)flashDev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb0_Flash_Lock(whal_Flash *flashDev, size_t addr,
                                    size_t len)
{
    (void)flashDev;
    (void)addr;
    (void)len;
    /* The WB0 flash controller has no software lock register — all
     * writes are gated only by the per-page PAGEPROT bits. */
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb0_Flash_Unlock(whal_Flash *flashDev, size_t addr,
                                      size_t len)
{
    (void)flashDev;
    (void)addr;
    (void)len;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb0_Flash_Read(whal_Flash *flashDev, size_t addr,
                                    void *data, size_t dataSz)
{
    const whal_Stm32wb0_Flash_Cfg *cfg =
        (const whal_Stm32wb0_Flash_Cfg *)whal_Stm32wb0_Flash_Dev.cfg;
    uint8_t *dataBuf = (uint8_t *)data;
    (void)flashDev;

    if (!data)
        return WHAL_EINVAL;
    if (dataSz == 0)
        return WHAL_SUCCESS;
    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Flash is memory-mapped; a simple byte copy is fine. */
    uint8_t *src = (uint8_t *)addr;
    for (size_t i = 0; i < dataSz; ++i)
        dataBuf[i] = src[i];
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb0_Flash_Write(whal_Flash *flashDev, size_t addr,
                                     const void *data, size_t dataSz)
{
    const whal_Stm32wb0_Flash_Cfg *cfg =
        (const whal_Stm32wb0_Flash_Cfg *)whal_Stm32wb0_Flash_Dev.cfg;
    size_t base = whal_Stm32wb0_Flash_Dev.base;
    const uint8_t *dataBuf = (const uint8_t *)data;
    whal_Error err;
    (void)flashDev;

    if (!data)
        return WHAL_EINVAL;
    if (dataSz == 0)
        return WHAL_SUCCESS;
    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;
    /* The WRITE command programs one 32-bit word at a time; address and
     * size must be word-aligned. */
    if ((addr & 0x3u) || (dataSz & 0x3u))
        return WHAL_EINVAL;

    for (size_t i = 0; i < dataSz; i += FLASH_WORD_SIZE) {
        uint32_t word;
        /* Copy bytewise to avoid relying on dataBuf alignment. */
        word = ((uint32_t)dataBuf[i + 0])       |
               ((uint32_t)dataBuf[i + 1] << 8)  |
               ((uint32_t)dataBuf[i + 2] << 16) |
               ((uint32_t)dataBuf[i + 3] << 24);

        whal_Reg_Write(base, FLASH_ADDRESS_REG,
                       whal_Stm32wb0_Flash_AddrToHw(cfg, addr + i));
        whal_Reg_Write(base, FLASH_DATA0_REG, word);

        err = whal_Stm32wb0_Flash_RunCommand(base, FLASH_COMMAND_WRITE,
                                             cfg->timeout);
        if (err)
            return err;
    }
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb0_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                     size_t dataSz)
{
    const whal_Stm32wb0_Flash_Cfg *cfg =
        (const whal_Stm32wb0_Flash_Cfg *)whal_Stm32wb0_Flash_Dev.cfg;
    size_t base = whal_Stm32wb0_Flash_Dev.base;
    whal_Error err;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_SUCCESS;
    if (addr < cfg->startAddr || addr + dataSz > cfg->startAddr + cfg->size)
        return WHAL_EINVAL;

    /* Round to the page that contains addr, then erase all pages that
     * intersect [addr, addr+dataSz). */
    size_t firstPage = (addr - cfg->startAddr) / FLASH_PAGE_SIZE;
    size_t lastPage  = (addr - cfg->startAddr + dataSz - 1) / FLASH_PAGE_SIZE;

    for (size_t p = firstPage; p <= lastPage; ++p) {
        /* For ERASE, ADDRESS[15:9] = page number, lower bits zero. */
        uint32_t hwAddr = (uint32_t)(p * FLASH_PAGE_SIZE / FLASH_WORD_SIZE);
        whal_Reg_Write(base, FLASH_ADDRESS_REG, hwAddr);

        err = whal_Stm32wb0_Flash_RunCommand(base, FLASH_COMMAND_ERASE,
                                             cfg->timeout);
        if (err)
            return err;
    }
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb0_Flash_Ext_SetLatency(whal_Flash *flashDev,
    enum whal_Stm32wb0_Flash_Latency latency)
{
    size_t base = whal_Stm32wb0_Flash_Dev.base;
    (void)flashDev;

    whal_Reg_Update(base, FLASH_CONFIG_REG, FLASH_CONFIG_WAIT_STATES_Msk,
                    whal_SetBits(FLASH_CONFIG_WAIT_STATES_Msk,
                                 FLASH_CONFIG_WAIT_STATES_Pos, latency));
    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_STM32WB0_FLASH_DIRECT_API_MAPPING
const whal_FlashDriver whal_Stm32wb0_Flash_Driver = {
    .Init   = whal_Stm32wb0_Flash_Init,
    .Deinit = whal_Stm32wb0_Flash_Deinit,
    .Lock   = whal_Stm32wb0_Flash_Lock,
    .Unlock = whal_Stm32wb0_Flash_Unlock,
    .Read   = whal_Stm32wb0_Flash_Read,
    .Write  = whal_Stm32wb0_Flash_Write,
    .Erase  = whal_Stm32wb0_Flash_Erase,
};
#endif /* !WHAL_CFG_STM32WB0_FLASH_DIRECT_API_MAPPING */
