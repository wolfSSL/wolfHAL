/* spi_nor_flash.c
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
#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_SpiNor_Dev singleton */
#endif
#include <wolfHAL/flash/spi_nor_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/error.h>
#include <wolfHAL/timeout.h>

/* SPI-NOR JEDEC Standard Commands */
#define CMD_WRITE_ENABLE  0x06
#define CMD_WRITE_DISABLE 0x04
#define CMD_READ_SR1      0x05 /* Read Status Register 1 */
#define CMD_WRITE_SR      0x01 /* Write Status Register */
#define CMD_READ_DATA     0x03
#define CMD_FAST_READ     0x0B
#define CMD_PAGE_PROGRAM  0x02
#define CMD_SECTOR_ERASE  0x20 /* 4 KB sector erase */
#define CMD_BLOCK_ERASE32 0x52 /* 32 KB block erase */
#define CMD_BLOCK_ERASE64 0xD8 /* 64 KB block erase */
#define CMD_CHIP_ERASE    0xC7

#define ERASE_SZ_4K    (4 * 1024)
#define ERASE_SZ_32K   (32 * 1024)
#define ERASE_SZ_64K   (64 * 1024)
#define EXREG_BANK_SZ  (16 * 1024 * 1024) /* 16 MB per EAR bank */
#define CMD_READ_JEDEC_ID    0x9F
#define CMD_RELEASE_PD       0xAB /* Release from Deep Power-Down */
#define CMD_ENTER_4B_MODE    0xB7

/* Dedicated 4-byte address commands */
#define CMD_READ_DATA_4B     0x13
#define CMD_FAST_READ_4B     0x0C
#define CMD_PAGE_PROGRAM_4B  0x12
#define CMD_SECTOR_ERASE_4B  0x21 /* 4 KB sector erase */
#define CMD_BLOCK_ERASE64_4B 0xDC /* 64 KB block erase */

/* Extended Address Register */
#define CMD_WRITE_EAR 0xC5
#define CMD_READ_EAR  0xC8

/* Status Register 1 Bits */
#define SR1_WIP  0x01 /* Write In Progress */
#define SR1_WEL  0x02 /* Write Enable Latch */
#define SR1_BP0  0x04 /* Block Protect bit 0 */
#define SR1_BP1  0x08 /* Block Protect bit 1 */
#define SR1_BP2  0x10 /* Block Protect bit 2 */
#define SR1_BP_MASK (SR1_BP0 | SR1_BP1 | SR1_BP2)

#define DUMMY 0xFF

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
const whal_Flash whal_SpiNor_Dev = WHAL_CFG_SPI_NOR_DEV;
#endif

static whal_Error SpiNor_CsAssert(whal_SpiNor_Cfg *cfg)
{
    return whal_Gpio_Set(cfg->gpioDev, cfg->csPin, 0);
}

static whal_Error SpiNor_CsDeassert(whal_SpiNor_Cfg *cfg)
{
    return whal_Gpio_Set(cfg->gpioDev, cfg->csPin, 1);
}

/* Send a 1-byte command with no data phase */
static whal_Error SpiNor_Cmd(whal_SpiNor_Cfg *cfg, uint8_t cmd)
{
    whal_Error err;

    err = SpiNor_CsAssert(cfg);
    if (err)
        return err;
    err = whal_Spi_SendRecv(cfg->spiDev, &cmd, 1, NULL, 0);
    SpiNor_CsDeassert(cfg);
    return err;
}

/* Read Status Register 1 */
static whal_Error SpiNor_ReadSR1(whal_SpiNor_Cfg *cfg, uint8_t *sr)
{
    uint8_t cmd = CMD_READ_SR1;
    whal_Error err;

    err = SpiNor_CsAssert(cfg);
    if (err)
        return err;
    err = whal_Spi_SendRecv(cfg->spiDev, &cmd, 1, NULL, 0);
    if (!err)
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, sr, 1);
    SpiNor_CsDeassert(cfg);
    return err;
}

/* Poll until WIP bit clears */
static whal_Error SpiNor_WaitReady(whal_SpiNor_Cfg *cfg)
{
    uint8_t sr;
    whal_Error err;

    WHAL_TIMEOUT_START(cfg->timeout);
    do {
        err = SpiNor_ReadSR1(cfg, &sr);
        if (err)
            return err;
        if (!(sr & SR1_WIP))
            return WHAL_SUCCESS;
        if (WHAL_TIMEOUT_EXPIRED(cfg->timeout))
            return WHAL_ETIMEOUT;
    } while (1);
}

/* Issue Write Enable (WREN) and verify WEL is set */
static whal_Error SpiNor_WriteEnable(whal_SpiNor_Cfg *cfg)
{
    uint8_t sr;
    whal_Error err;

    err = SpiNor_Cmd(cfg, CMD_WRITE_ENABLE);
    if (err)
        return err;

    err = SpiNor_ReadSR1(cfg, &sr);
    if (err)
        return err;
    if (!(sr & SR1_WEL))
        return WHAL_EHARDWARE;

    return WHAL_SUCCESS;
}

/* Build a command + 3-byte address frame */
static void SpiNor_BuildCmdAddr(uint8_t *frame, uint8_t cmd, size_t addr)
{
    frame[0] = cmd;
    frame[1] = (uint8_t)(addr >> 16);
    frame[2] = (uint8_t)(addr >> 8);
    frame[3] = (uint8_t)(addr);
}

/* Build a command + 4-byte address frame */
static void SpiNor_BuildCmdAddr4b(uint8_t *frame, uint8_t cmd, size_t addr)
{
    frame[0] = cmd;
    frame[1] = (uint8_t)(addr >> 24);
    frame[2] = (uint8_t)(addr >> 16);
    frame[3] = (uint8_t)(addr >> 8);
    frame[4] = (uint8_t)(addr);
}

whal_Error whal_SpiNor_Init(whal_Flash *flashDev)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t jedec[3];
    uint8_t cmd;
    whal_Error err;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;
#else
    if (!flashDev || !flashDev->cfg)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (!cfg->spiDev || !cfg->spiComCfg || !cfg->gpioDev)
        return WHAL_EINVAL;
    if (cfg->pageSz == 0 || cfg->capacity == 0)
        return WHAL_EINVAL;

    /* Ensure CS is deasserted */
    err = SpiNor_CsDeassert(cfg);
    if (err)
        return err;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    /* Wake device in case it was left in Deep Power-Down */
    err = SpiNor_Cmd(cfg, CMD_RELEASE_PD);
    if (err)
        goto cleanup;

    /* Wait for any in-progress operation from a previous session */
    err = SpiNor_WaitReady(cfg);
    if (err)
        goto cleanup;

    /* Read JEDEC ID to verify a device is present */
    cmd = CMD_READ_JEDEC_ID;
    err = SpiNor_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = whal_Spi_SendRecv(cfg->spiDev, &cmd, 1, NULL, 0);
    if (!err)
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, jedec, 3);
    SpiNor_CsDeassert(cfg);
    if (err)
        goto cleanup;

    /* A manufacturer ID of 0x00 or 0xFF indicates no device */
    if (jedec[0] == 0x00 || jedec[0] == 0xFF) {
        err = WHAL_EHARDWARE;
        goto cleanup;
    }

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor_Deinit(whal_Flash *flashDev)
{
    whal_SpiNor_Cfg *cfg;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;
#else
    if (!flashDev || !flashDev->cfg)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    return SpiNor_CsDeassert(cfg);
}

whal_Error whal_SpiNor_Lock(whal_Flash *flashDev, size_t addr, size_t len)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[2];
    whal_Error err;

    (void)addr;
    (void)len;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;
#else
    if (!flashDev || !flashDev->cfg)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SpiNor_WaitReady(cfg);
    if (err)
        goto cleanup;

    /* Set all block protect bits to lock entire device */
    err = SpiNor_WriteEnable(cfg);
    if (err)
        goto cleanup;

    frame[0] = CMD_WRITE_SR;
    frame[1] = SR1_BP_MASK;
    err = SpiNor_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = whal_Spi_SendRecv(cfg->spiDev, frame, 2, NULL, 0);
    SpiNor_CsDeassert(cfg);
    if (!err)
        err = SpiNor_WaitReady(cfg);

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor_Unlock(whal_Flash *flashDev, size_t addr, size_t len)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[2];
    whal_Error err;

    (void)addr;
    (void)len;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;
#else
    if (!flashDev || !flashDev->cfg)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SpiNor_WaitReady(cfg);
    if (err)
        goto cleanup;

    /* Clear all block protect bits to unlock entire device */
    err = SpiNor_WriteEnable(cfg);
    if (err)
        goto cleanup;

    frame[0] = CMD_WRITE_SR;
    frame[1] = 0x00;
    err = SpiNor_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = whal_Spi_SendRecv(cfg->spiDev, frame, 2, NULL, 0);
    SpiNor_CsDeassert(cfg);
    if (!err)
        err = SpiNor_WaitReady(cfg);

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor3b_Read(whal_Flash *flashDev, size_t addr, void *data,
                            size_t dataSz)
{
    uint8_t *dataBuf = (uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[4];
    whal_Error err;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr & ~0xFFFFFF || addr >= cfg->capacity ||
        dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SpiNor_WaitReady(cfg);
    if (err)
        goto cleanup;

    SpiNor_BuildCmdAddr(frame, CMD_READ_DATA, addr);
    err = SpiNor_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = whal_Spi_SendRecv(cfg->spiDev, frame, 4, NULL, 0);
    if (!err)
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, dataBuf, dataSz);
    SpiNor_CsDeassert(cfg);

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor3b_Write(whal_Flash *flashDev, size_t addr,
                             const void *data, size_t dataSz)
{
    const uint8_t *dataBuf = (const uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[4];
    whal_Error err;
    size_t chunk;
    size_t offset;
    size_t pageRemaining;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr & ~0xFFFFFF || addr >= cfg->capacity ||
        dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    /*
     * Page Program cannot cross page boundaries. Split writes so each
     * transaction stays within a single page.
     */
    offset = 0;
    while (offset < dataSz) {
        pageRemaining = cfg->pageSz - ((addr + offset) & (cfg->pageSz - 1));
        chunk = dataSz - offset;
        if (chunk > pageRemaining)
            chunk = pageRemaining;

        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr(frame, CMD_PAGE_PROGRAM, addr + offset);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 4, NULL, 0);
        if (!err)
            err = whal_Spi_SendRecv(cfg->spiDev, dataBuf + offset, chunk,
                                    NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        offset += chunk;
    }

    /* Wait for final page program to complete */
    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor3b_Erase4k(whal_Flash *flashDev, size_t addr,
                               size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[4];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr & ~0xFFFFFF || addr >= cfg->capacity ||
        dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_4K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr(frame, CMD_SECTOR_ERASE, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 4, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_4K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor3b_Erase32k(whal_Flash *flashDev, size_t addr,
                                size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[4];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr & ~0xFFFFFF || addr >= cfg->capacity ||
        dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_32K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr(frame, CMD_BLOCK_ERASE32, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 4, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_32K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor3b_Erase64k(whal_Flash *flashDev, size_t addr,
                                size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[4];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr & ~0xFFFFFF || addr >= cfg->capacity ||
        dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_64K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr(frame, CMD_BLOCK_ERASE64, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 4, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_64K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor_EraseChip(whal_Flash *flashDev, size_t addr,
                                 size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    whal_Error err;

    (void)addr;
    (void)dataSz;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;
#else
    if (!flashDev || !flashDev->cfg)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SpiNor_WaitReady(cfg);
    if (!err)
        err = SpiNor_WriteEnable(cfg);
    if (!err)
        err = SpiNor_Cmd(cfg, CMD_CHIP_ERASE);
    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

/* --- Fast Read (3-byte addr) --- */

whal_Error whal_SpiNor3b_ReadFast(whal_Flash *flashDev, size_t addr,
                                void *data, size_t dataSz)
{
    uint8_t *dataBuf = (uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr & ~0xFFFFFF || addr >= cfg->capacity ||
        dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SpiNor_WaitReady(cfg);
    if (err)
        goto cleanup;

    SpiNor_BuildCmdAddr(frame, CMD_FAST_READ, addr);
    frame[4] = DUMMY;
    err = SpiNor_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
    if (!err)
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, dataBuf, dataSz);
    SpiNor_CsDeassert(cfg);

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

/* --- Dedicated 4-byte address commands --- */

whal_Error whal_SpiNor4bMode_Init(whal_Flash *flashDev)
{
    whal_SpiNor_Cfg *cfg;
    whal_Error err;

    err = whal_SpiNor_Init(flashDev);
    if (err)
        return err;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;
#else
    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SpiNor_Cmd(cfg, CMD_ENTER_4B_MODE);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4b_Read(whal_Flash *flashDev, size_t addr,
                              void *data, size_t dataSz)
{
    uint8_t *dataBuf = (uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SpiNor_WaitReady(cfg);
    if (err)
        goto cleanup;

    SpiNor_BuildCmdAddr4b(frame, CMD_READ_DATA_4B, addr);
    err = SpiNor_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
    if (!err)
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, dataBuf, dataSz);
    SpiNor_CsDeassert(cfg);

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4b_ReadFast(whal_Flash *flashDev, size_t addr,
                                  void *data, size_t dataSz)
{
    uint8_t *dataBuf = (uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[6];
    whal_Error err;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SpiNor_WaitReady(cfg);
    if (err)
        goto cleanup;

    SpiNor_BuildCmdAddr4b(frame, CMD_FAST_READ_4B, addr);
    frame[5] = DUMMY;
    err = SpiNor_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = whal_Spi_SendRecv(cfg->spiDev, frame, 6, NULL, 0);
    if (!err)
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, dataBuf, dataSz);
    SpiNor_CsDeassert(cfg);

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4b_Write(whal_Flash *flashDev, size_t addr,
                               const void *data, size_t dataSz)
{
    const uint8_t *dataBuf = (const uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;
    size_t chunk;
    size_t offset;
    size_t pageRemaining;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    offset = 0;
    while (offset < dataSz) {
        pageRemaining = cfg->pageSz - ((addr + offset) & (cfg->pageSz - 1));
        chunk = dataSz - offset;
        if (chunk > pageRemaining)
            chunk = pageRemaining;

        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr4b(frame, CMD_PAGE_PROGRAM_4B, addr + offset);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
        if (!err)
            err = whal_Spi_SendRecv(cfg->spiDev, dataBuf + offset, chunk,
                                    NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        offset += chunk;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4b_Erase4k(whal_Flash *flashDev, size_t addr,
                                  size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_4K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr4b(frame, CMD_SECTOR_ERASE_4B, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_4K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4b_Erase64k(whal_Flash *flashDev, size_t addr,
                                   size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_64K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr4b(frame, CMD_BLOCK_ERASE64_4B, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_64K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

/* --- 4-byte address mode (0xB7) variants --- */

whal_Error whal_SpiNor4bMode_Read(whal_Flash *flashDev, size_t addr,
                                  void *data, size_t dataSz)
{
    uint8_t *dataBuf = (uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SpiNor_WaitReady(cfg);
    if (err)
        goto cleanup;

    SpiNor_BuildCmdAddr4b(frame, CMD_READ_DATA, addr);
    err = SpiNor_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
    if (!err)
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, dataBuf, dataSz);
    SpiNor_CsDeassert(cfg);

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4bMode_ReadFast(whal_Flash *flashDev, size_t addr,
                                      void *data, size_t dataSz)
{
    uint8_t *dataBuf = (uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[6];
    whal_Error err;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SpiNor_WaitReady(cfg);
    if (err)
        goto cleanup;

    SpiNor_BuildCmdAddr4b(frame, CMD_FAST_READ, addr);
    frame[5] = DUMMY;
    err = SpiNor_CsAssert(cfg);
    if (err)
        goto cleanup;
    err = whal_Spi_SendRecv(cfg->spiDev, frame, 6, NULL, 0);
    if (!err)
        err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, dataBuf, dataSz);
    SpiNor_CsDeassert(cfg);

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4bMode_Write(whal_Flash *flashDev, size_t addr,
                                   const void *data, size_t dataSz)
{
    const uint8_t *dataBuf = (const uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;
    size_t chunk;
    size_t offset;
    size_t pageRemaining;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    offset = 0;
    while (offset < dataSz) {
        pageRemaining = cfg->pageSz - ((addr + offset) & (cfg->pageSz - 1));
        chunk = dataSz - offset;
        if (chunk > pageRemaining)
            chunk = pageRemaining;

        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr4b(frame, CMD_PAGE_PROGRAM, addr + offset);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
        if (!err)
            err = whal_Spi_SendRecv(cfg->spiDev, dataBuf + offset, chunk,
                                    NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        offset += chunk;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4bMode_Erase4k(whal_Flash *flashDev, size_t addr,
                                      size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_4K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr4b(frame, CMD_SECTOR_ERASE, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_4K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4bMode_Erase32k(whal_Flash *flashDev, size_t addr,
                                       size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_32K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr4b(frame, CMD_BLOCK_ERASE32, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_32K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4bMode_Erase64k(whal_Flash *flashDev, size_t addr,
                                       size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_64K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr4b(frame, CMD_BLOCK_ERASE64, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_64K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

/* --- Extended Address Register variants --- */

/*
 * Set the extended address register to select the 16 MB bank containing
 * the given address.
 */
static whal_Error SpiNor_SetEAR(whal_SpiNor_Cfg *cfg, size_t addr)
{
    uint8_t frame[2];
    whal_Error err;

    err = SpiNor_WriteEnable(cfg);
    if (err)
        return err;

    frame[0] = CMD_WRITE_EAR;
    frame[1] = (uint8_t)(addr >> 24);
    err = SpiNor_CsAssert(cfg);
    if (err)
        return err;
    err = whal_Spi_SendRecv(cfg->spiDev, frame, 2, NULL, 0);
    SpiNor_CsDeassert(cfg);
    return err;
}

whal_Error whal_SpiNor4bExReg_Read(whal_Flash *flashDev, size_t addr,
                                   void *data, size_t dataSz)
{
    uint8_t *dataBuf = (uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[4];
    whal_Error err;
    size_t offset;
    size_t chunk;
    size_t bankRemaining;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    offset = 0;
    while (offset < dataSz) {
        bankRemaining = EXREG_BANK_SZ -
                        ((addr + offset) & (EXREG_BANK_SZ - 1));
        chunk = dataSz - offset;
        if (chunk > bankRemaining)
            chunk = bankRemaining;

        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_SetEAR(cfg, addr + offset);
        if (err)
            break;

        SpiNor_BuildCmdAddr(frame, CMD_READ_DATA, addr + offset);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 4, NULL, 0);
        if (!err)
            err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, dataBuf + offset,
                                    chunk);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        offset += chunk;
    }

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4bExReg_ReadFast(whal_Flash *flashDev, size_t addr,
                                       void *data, size_t dataSz)
{
    uint8_t *dataBuf = (uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[5];
    whal_Error err;
    size_t offset;
    size_t chunk;
    size_t bankRemaining;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    offset = 0;
    while (offset < dataSz) {
        bankRemaining = EXREG_BANK_SZ -
                        ((addr + offset) & (EXREG_BANK_SZ - 1));
        chunk = dataSz - offset;
        if (chunk > bankRemaining)
            chunk = bankRemaining;

        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_SetEAR(cfg, addr + offset);
        if (err)
            break;

        SpiNor_BuildCmdAddr(frame, CMD_FAST_READ, addr + offset);
        frame[4] = DUMMY;
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 5, NULL, 0);
        if (!err)
            err = whal_Spi_SendRecv(cfg->spiDev, NULL, 0, dataBuf + offset,
                                    chunk);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        offset += chunk;
    }

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4bExReg_Write(whal_Flash *flashDev, size_t addr,
                                    const void *data, size_t dataSz)
{
    const uint8_t *dataBuf = (const uint8_t *)data;
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[4];
    whal_Error err;
    size_t chunk;
    size_t offset;
    size_t pageRemaining;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (!data || dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || !data || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    offset = 0;
    while (offset < dataSz) {
        pageRemaining = cfg->pageSz - ((addr + offset) & (cfg->pageSz - 1));
        chunk = dataSz - offset;
        if (chunk > pageRemaining)
            chunk = pageRemaining;

        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_SetEAR(cfg, addr + offset);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr(frame, CMD_PAGE_PROGRAM, addr + offset);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 4, NULL, 0);
        if (!err)
            err = whal_Spi_SendRecv(cfg->spiDev, dataBuf + offset, chunk,
                                    NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        offset += chunk;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4bExReg_Erase4k(whal_Flash *flashDev, size_t addr,
                                       size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[4];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_4K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_SetEAR(cfg, cur);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr(frame, CMD_SECTOR_ERASE, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 4, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_4K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4bExReg_Erase32k(whal_Flash *flashDev, size_t addr,
                                        size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[4];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_32K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_SetEAR(cfg, cur);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr(frame, CMD_BLOCK_ERASE32, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 4, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_32K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SpiNor4bExReg_Erase64k(whal_Flash *flashDev, size_t addr,
                                        size_t dataSz)
{
    whal_SpiNor_Cfg *cfg;
    uint8_t frame[4];
    whal_Error err;
    size_t end;
    size_t cur;

#ifdef WHAL_CFG_SPI_NOR_SINGLE_INSTANCE
    cfg = (whal_SpiNor_Cfg *)whal_SpiNor_Dev.cfg;
    (void)flashDev;

    if (dataSz == 0)
        return WHAL_EINVAL;
#else
    if (!flashDev || !flashDev->cfg || dataSz == 0)
        return WHAL_EINVAL;

    cfg = (whal_SpiNor_Cfg *)flashDev->cfg;
#endif

    if (addr >= cfg->capacity || dataSz > cfg->capacity - addr)
        return WHAL_EINVAL;

    if ((addr | dataSz) & (ERASE_SZ_64K - 1))
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    end = addr + dataSz;
    cur = addr;

    while (cur < end) {
        err = SpiNor_WaitReady(cfg);
        if (err)
            break;

        err = SpiNor_SetEAR(cfg, cur);
        if (err)
            break;

        err = SpiNor_WriteEnable(cfg);
        if (err)
            break;

        SpiNor_BuildCmdAddr(frame, CMD_BLOCK_ERASE64, cur);
        err = SpiNor_CsAssert(cfg);
        if (err)
            break;
        err = whal_Spi_SendRecv(cfg->spiDev, frame, 4, NULL, 0);
        SpiNor_CsDeassert(cfg);
        if (err)
            break;

        cur += ERASE_SZ_64K;
    }

    if (!err)
        err = SpiNor_WaitReady(cfg);

    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

const whal_FlashDriver whal_SpiNor_Driver = {
    .Init = whal_SpiNor_Init,
    .Deinit = whal_SpiNor_Deinit,
    .Lock = whal_SpiNor_Lock,
    .Unlock = whal_SpiNor_Unlock,
    .Read = whal_SpiNor3b_Read,
    .Write = whal_SpiNor3b_Write,
    .Erase = whal_SpiNor3b_Erase4k,
};
