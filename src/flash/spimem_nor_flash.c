/* spimem_nor_flash.c
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
#ifdef WHAL_CFG_SPIMEM_NOR_FLASH_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides whal_SpiMemNor_Flash_Dev singleton */
#endif
#include <wolfHAL/flash/spimem_nor_flash.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/spimem/spimem.h>
#include <wolfHAL/error.h>
#include <wolfHAL/timeout.h>

/* Commands */
#define CMD_WRITE_ENABLE 0x06 /* sets the write enable latch */
#define CMD_READ_SR      0x05 /* read status register */
#define CMD_READ_ID      0x9F /* JEDEC ID: manufacturer, type, capacity */
#define CMD_RESET_ENABLE 0x66 /* reset enable, then RESET MEMORY */
#define CMD_RESET_MEMORY 0x99 /* reset to the nonvolatile settings */

#define READ_ID_SZ 3 /* bytes of the JEDEC ID read */

/* Status register bits */
#define SR_BUSY 0x01 /* bit 0: write in progress */
#define SR_WEL  0x02 /* bit 1: write enable latch */

#ifdef WHAL_CFG_SPIMEM_NOR_FLASH_DIRECT_API_MAPPING
#define whal_SpiMemNor_Flash_Init   whal_Flash_Init
#define whal_SpiMemNor_Flash_Deinit whal_Flash_Deinit
#define whal_SpiMemNor_Flash_Lock   whal_Flash_Lock
#define whal_SpiMemNor_Flash_Unlock whal_Flash_Unlock
#define whal_SpiMemNor_Flash_Read   whal_Flash_Read
#define whal_SpiMemNor_Flash_Write  whal_Flash_Write
#define whal_SpiMemNor_Flash_Erase  whal_Flash_Erase
#endif /* WHAL_CFG_SPIMEM_NOR_FLASH_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_SPIMEM_NOR_FLASH_SINGLE_INSTANCE
const whal_Flash whal_SpiMemNor_Flash_Dev = WHAL_CFG_SPIMEM_NOR_FLASH_DEV;
#endif

/* Get the configuration of a device */
static inline whal_Error SpiMemNor_GetCfg(whal_Flash *flashDev,
                                          whal_SpiMemNor_Flash_Cfg **cfg)
{
#ifdef WHAL_CFG_SPIMEM_NOR_FLASH_SINGLE_INSTANCE
    (void)flashDev;
    *cfg = (whal_SpiMemNor_Flash_Cfg *)whal_SpiMemNor_Flash_Dev.cfg;
#else
    if (!flashDev || !flashDev->cfg) {
        return WHAL_EINVAL;
    }
    *cfg = (whal_SpiMemNor_Flash_Cfg *)flashDev->cfg;
#endif
    return WHAL_SUCCESS;
}

/* Check that addr..addr+size is inside the flash */
static inline whal_Error SpiMemNor_CheckRange(whal_SpiMemNor_Flash_Cfg *cfg,
                                              size_t addr, size_t size)
{
    if (addr > cfg->capacity || size > cfg->capacity - addr) {
        return WHAL_EINVAL;
    }
    return WHAL_SUCCESS;
}

/* Send a command with no address and no data */
static inline whal_Error SpiMemNor_Cmd(whal_SpiMemNor_Flash_Cfg *cfg,
                                       uint8_t opcode)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;

    op.opcode = opcode;
    op.dir = WHAL_SPIMEM_DIR_NONE;
    return whal_SpiMem_Exec(cfg->spiMemDev, &op);
}

/* Read a one byte register (status or error) */
static inline whal_Error SpiMemNor_ReadReg(whal_SpiMemNor_Flash_Cfg *cfg,
                                           uint8_t opcode, uint8_t *val)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;

    op.opcode = opcode;
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.data = val;
    op.dataSz = 1;
    return whal_SpiMem_Exec(cfg->spiMemDev, &op);
}

/* Poll the status register until (sr & mask) == value */
static whal_Error SpiMemNor_PollSr(whal_SpiMemNor_Flash_Cfg *cfg,
                                   uint8_t mask, uint8_t value)
{
    uint8_t sr;
    whal_Error err;

    WHAL_TIMEOUT_START(cfg->timeout);
    while (1) {
        sr = 0;
        err = SpiMemNor_ReadReg(cfg, CMD_READ_SR, &sr);
        if (err) {
            return err;
        }
        if ((sr & mask) == value) {
            return WHAL_SUCCESS;
        }
        if (WHAL_TIMEOUT_EXPIRED(cfg->timeout)) {
            return WHAL_ETIMEOUT;
        }
    }
}

/* WRITE ENABLE, then wait for the latch and not busy. A busy flash ignores
 * commands, so a lost WRITE ENABLE shows up here. */
static whal_Error SpiMemNor_WriteEnable(whal_SpiMemNor_Flash_Cfg *cfg)
{
    whal_Error err;

    err = SpiMemNor_Cmd(cfg, CMD_WRITE_ENABLE);
    if (err) {
        return err;
    }
    return SpiMemNor_PollSr(cfg, SR_WEL | SR_BUSY, SR_WEL);
}

/* With errOp: fail if the last program or erase set an errMask bit, and
 * clear the bits so the next operation starts clean */
static whal_Error SpiMemNor_CheckErr(whal_SpiMemNor_Flash_Cfg *cfg)
{
    uint8_t val = 0;
    whal_Error err;

    if (cfg->errOp == 0) {
        return WHAL_SUCCESS;
    }
    err = SpiMemNor_ReadReg(cfg, cfg->errOp, &val);
    if (err) {
        return err;
    }
    if ((val & cfg->errMask) == 0) {
        return WHAL_SUCCESS;
    }
    if (cfg->errClearOp != 0) {
        /* Best effort: the program or erase error is the one to report */
        (void)SpiMemNor_Cmd(cfg, cfg->errClearOp);
    }
    return WHAL_EHARDWARE;
}

/* With rstRegOp: WREN, write one byte to rstRegAddr, wait */
static whal_Error SpiMemNor_WriteRstReg(whal_SpiMemNor_Flash_Cfg *cfg)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    uint8_t val = cfg->rstRegVal;
    whal_Error err;

    if (cfg->rstRegOp == 0) {
        return WHAL_SUCCESS;
    }
    err = SpiMemNor_WriteEnable(cfg);
    if (err) {
        return err;
    }
    op.opcode = cfg->rstRegOp;
    op.addrBytes = cfg->rstRegAddrBytes;
    op.addr = cfg->rstRegAddr;
    op.dir = WHAL_SPIMEM_DIR_OUT;
    op.data = &val;
    op.dataSz = 1;
    err = whal_SpiMem_Exec(cfg->spiMemDev, &op);
    if (err) {
        return err;
    }
    return SpiMemNor_PollSr(cfg, SR_BUSY, 0);
}

/* Read the 3-byte JEDEC ID in 1-bit mode */
static whal_Error SpiMemNor_ReadJedecId(whal_SpiMemNor_Flash_Cfg *cfg,
                                        uint8_t id[READ_ID_SZ])
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;

    op.opcode = CMD_READ_ID;
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.data = id;
    op.dataSz = READ_ID_SZ;
    return whal_SpiMem_Exec(cfg->spiMemDev, &op);
}

/* 1 if the ID is a flash (manufacturer not 0x00 or 0xFF) and, when cfg->id
 * is set, the expected one */
static int SpiMemNor_IdOk(whal_SpiMemNor_Flash_Cfg *cfg,
                          const uint8_t id[READ_ID_SZ])
{
    uint32_t got;

    if (id[0] == 0x00 || id[0] == 0xFF) {
        return 0;
    }
    if (cfg->id == 0) {
        return 1;
    }
    got = (uint32_t)id[0] | ((uint32_t)id[1] << 8) | ((uint32_t)id[2] << 16);
    if (got == cfg->id) {
        return 1;
    }
    return 0;
}

/* RESET ENABLE then RESET MEMORY. With octal set, as 8-line DTR commands
 * with no address or data (8D-0-0). In octal DDR the command lasts a whole
 * clock, the byte on both edges, so it goes out twice. */
static whal_Error SpiMemNor_ResetCmds(whal_SpiMemNor_Flash_Cfg *cfg,
                                      int octal)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    whal_Error err;

    op.dir = WHAL_SPIMEM_DIR_NONE;
    op.opcode = CMD_RESET_ENABLE;
    if (octal) {
        op.opLines = WHAL_SPIMEM_LINES_8;
        op.dtr = 1;
        op.opBytes = 2;
        op.opcode = (CMD_RESET_ENABLE << 8) | CMD_RESET_ENABLE;
    }
    err = whal_SpiMem_Exec(cfg->spiMemDev, &op);
    if (err) {
        return err;
    }
    op.opcode = CMD_RESET_MEMORY;
    if (octal) {
        op.opcode = (CMD_RESET_MEMORY << 8) | CMD_RESET_MEMORY;
    }
    return whal_SpiMem_Exec(cfg->spiMemDev, &op);
}

/* Back to 1-bit: octal reset (errors ignored), 1-bit reset, wait, read ID;
 * if the ID matches, the rstReg write. */
static whal_Error SpiMemNor_Recover(whal_SpiMemNor_Flash_Cfg *cfg,
                                    uint8_t id[READ_ID_SZ])
{
    whal_Error err;

    if (cfg->flags & WHAL_SPIMEM_NOR_FLAG_RESET_OCTAL_DTR) {
        /* A controller without octal DTR refuses this; the 1-bit reset
         * below still runs and its result counts. */
        (void)SpiMemNor_ResetCmds(cfg, 1);
    }
    err = SpiMemNor_ResetCmds(cfg, 0);
    if (err) {
        return err;
    }
    err = SpiMemNor_PollSr(cfg, SR_BUSY, 0);
    if (err) {
        return err;
    }
    err = SpiMemNor_ReadJedecId(cfg, id);
    if (err) {
        return err;
    }
    if (!SpiMemNor_IdOk(cfg, id)) {
        return WHAL_SUCCESS;
    }
    return SpiMemNor_WriteRstReg(cfg);
}

/* Wait for the end of a program or erase, then check for errors */
static whal_Error SpiMemNor_WaitDone(whal_SpiMemNor_Flash_Cfg *cfg)
{
    whal_Error err;

    err = SpiMemNor_PollSr(cfg, SR_BUSY, 0);
    if (err) {
        return err;
    }
    return SpiMemNor_CheckErr(cfg);
}

whal_Error whal_SpiMemNor_Flash_Init(whal_Flash *flashDev)
{
    whal_SpiMemNor_Flash_Cfg *cfg;
    uint8_t id[READ_ID_SZ] = {0};
    uint64_t lastAddr;
    whal_Error err;

    err = SpiMemNor_GetCfg(flashDev, &cfg);
    if (err) {
        return err;
    }

    /* Writes and erases split with masks: sizes are powers of two, 2 or
     * more. spiMemDev may be WHAL_INTERNAL_DEV (NULL), so it is not checked. */
    if (cfg->capacity == 0) {
        return WHAL_EINVAL;
    }
    if (cfg->pageSz < 2 || (cfg->pageSz & (cfg->pageSz - 1)) != 0) {
        return WHAL_EINVAL;
    }
    if (cfg->eraseSz < 2 || (cfg->eraseSz & (cfg->eraseSz - 1)) != 0) {
        return WHAL_EINVAL;
    }
    if (cfg->addrBytes != 3 && cfg->addrBytes != 4) {
        return WHAL_EINVAL;
    }
    /* The last address must fit in addrBytes, or commands for high
     * addresses would go to low ones. */
    lastAddr = cfg->capacity - 1;
    if ((lastAddr >> (8 * cfg->addrBytes)) != 0) {
        return WHAL_EINVAL;
    }
    if (cfg->rstRegAddrBytes != 0 && cfg->rstRegAddrBytes != 3 &&
        cfg->rstRegAddrBytes != 4) {
        return WHAL_EINVAL;
    }

    /* Read the JEDEC ID to check that the flash answers in 1-bit mode */
    err = SpiMemNor_ReadJedecId(cfg, id);
    if (err) {
        return err;
    }

    /* No answer, or not the expected part: with FLAG_RESET, reset the
     * flash (it may be in another mode) and read the ID again */
    if (!SpiMemNor_IdOk(cfg, id) &&
        (cfg->flags & WHAL_SPIMEM_NOR_FLAG_RESET)) {
        err = SpiMemNor_Recover(cfg, id);
        if (err) {
            return err;
        }
    }

    /* Still no flash, or not the expected part */
    if (!SpiMemNor_IdOk(cfg, id)) {
        return WHAL_EHARDWARE;
    }

    /* Clear errors left from an earlier session */
    if (cfg->errClearOp != 0) {
        return SpiMemNor_Cmd(cfg, cfg->errClearOp);
    }

    return WHAL_SUCCESS;
}

whal_Error whal_SpiMemNor_Flash_ReadId(whal_Flash *flashDev, uint8_t id[3])
{
    whal_SpiMemNor_Flash_Cfg *cfg;
    whal_Error err;

    if (!id) {
        return WHAL_EINVAL;
    }
    err = SpiMemNor_GetCfg(flashDev, &cfg);
    if (err) {
        return err;
    }
    return SpiMemNor_ReadJedecId(cfg, id);
}

whal_Error whal_SpiMemNor_Flash_Deinit(whal_Flash *flashDev)
{
    (void)flashDev;

    /* Nothing to undo: the driver keeps no state */
    return WHAL_SUCCESS;
}

whal_Error whal_SpiMemNor_Flash_Lock(whal_Flash *flashDev, size_t addr,
                                     size_t len)
{
    (void)flashDev;
    (void)addr;
    (void)len;

    /* No protect commands are sent: the caller guards the flash */
    return WHAL_ENOTSUP;
}

whal_Error whal_SpiMemNor_Flash_Unlock(whal_Flash *flashDev, size_t addr,
                                       size_t len)
{
    (void)flashDev;
    (void)addr;
    (void)len;

    /* No protect commands are sent: the caller guards the flash */
    return WHAL_ENOTSUP;
}

/* Read through a window over exactly addr..addr+dataSz-1, closed again
 * even if the read fails. */
static whal_Error SpiMemNor_MapRead(whal_SpiMemNor_Flash_Cfg *cfg,
                                    const whal_SpiMemOp *op, size_t addr,
                                    void *data, size_t dataSz)
{
    whal_SpiMemMap map;
    whal_Error err;
    whal_Error err2;

    map.tmpl = *op;
    map.offset = addr;
    map.length = dataSz;
    err = whal_SpiMem_MapCreate(cfg->spiMemDev, &map);
    if (err) {
        return err;
    }
    err = whal_SpiMem_MapRead(cfg->spiMemDev, &map, 0, data, dataSz);
    err2 = whal_SpiMem_MapDestroy(cfg->spiMemDev, &map);
    if (err) {
        return err;
    }
    return err2;
}

whal_Error whal_SpiMemNor_Flash_Read(whal_Flash *flashDev, size_t addr,
                                     void *data, size_t dataSz)
{
    whal_SpiMemNor_Flash_Cfg *cfg;
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    whal_Error err;

    err = SpiMemNor_GetCfg(flashDev, &cfg);
    if (err) {
        return err;
    }
    if (dataSz == 0) {
        return WHAL_SUCCESS;
    }
    if (!data) {
        return WHAL_EINVAL;
    }
    err = SpiMemNor_CheckRange(cfg, addr, dataSz);
    if (err) {
        return err;
    }

    op.opcode = cfg->readOp;
    op.addrBytes = cfg->addrBytes;
    op.dummyCycles = cfg->readDummy;
    op.dir = WHAL_SPIMEM_DIR_IN;
    if (cfg->flags & WHAL_SPIMEM_NOR_FLAG_MAP_READ) {
        return SpiMemNor_MapRead(cfg, &op, addr, data, dataSz);
    }

    /* One read operation; the controller may split it */
    op.addr = (uint32_t)addr;
    op.data = data;
    op.dataSz = dataSz;
    return whal_SpiMem_Exec(cfg->spiMemDev, &op);
}

whal_Error whal_SpiMemNor_Flash_Write(whal_Flash *flashDev, size_t addr,
                                      const void *data, size_t dataSz)
{
    whal_SpiMemNor_Flash_Cfg *cfg;
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    const uint8_t *src = (const uint8_t *)data;
    size_t maxSz = 0;
    size_t chunk;
    whal_Error err;

    err = SpiMemNor_GetCfg(flashDev, &cfg);
    if (err) {
        return err;
    }
    if (dataSz == 0) {
        return WHAL_SUCCESS;
    }
    if (!data) {
        return WHAL_EINVAL;
    }
    err = SpiMemNor_CheckRange(cfg, addr, dataSz);
    if (err) {
        return err;
    }

    op.opcode = cfg->progOp;
    op.addrBytes = cfg->addrBytes;
    op.dir = WHAL_SPIMEM_DIR_OUT;
    op.addr = (uint32_t)addr;

    /* Largest program the controller can send in one command */
    err = whal_SpiMem_MaxDataSz(cfg->spiMemDev, &op, &maxSz);
    if (err) {
        return err;
    }
    if (maxSz == 0) {
        return WHAL_EINVAL;
    }

    while (dataSz > 0) {
        /* A program wraps inside its page, so stop each piece at the
         * page end */
        chunk = cfg->pageSz - (addr & (cfg->pageSz - 1));
        if (chunk > dataSz) {
            chunk = dataSz;
        }
        if (chunk > maxSz) {
            chunk = maxSz;
        }

        err = SpiMemNor_WriteEnable(cfg);
        if (err) {
            return err;
        }

        op.addr = (uint32_t)addr;
        op.data = (void *)src;
        op.dataSz = chunk;
        err = whal_SpiMem_Exec(cfg->spiMemDev, &op);
        if (err) {
            return err;
        }

        err = SpiMemNor_WaitDone(cfg);
        if (err) {
            return err;
        }

        addr += chunk;
        src += chunk;
        dataSz -= chunk;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_SpiMemNor_Flash_Erase(whal_Flash *flashDev, size_t addr,
                                      size_t dataSz)
{
    whal_SpiMemNor_Flash_Cfg *cfg;
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    whal_Error err;

    err = SpiMemNor_GetCfg(flashDev, &cfg);
    if (err) {
        return err;
    }
    if (dataSz == 0) {
        return WHAL_SUCCESS;
    }
    err = SpiMemNor_CheckRange(cfg, addr, dataSz);
    if (err) {
        return err;
    }
    if ((addr & (cfg->eraseSz - 1)) != 0 ||
        (dataSz & (cfg->eraseSz - 1)) != 0) {
        return WHAL_EINVAL;
    }

    op.opcode = cfg->eraseOp;
    op.addrBytes = cfg->addrBytes;
    op.dir = WHAL_SPIMEM_DIR_NONE;

    while (dataSz > 0) {
        err = SpiMemNor_WriteEnable(cfg);
        if (err) {
            return err;
        }

        op.addr = (uint32_t)addr;
        err = whal_SpiMem_Exec(cfg->spiMemDev, &op);
        if (err) {
            return err;
        }

        err = SpiMemNor_WaitDone(cfg);
        if (err) {
            return err;
        }

        addr += cfg->eraseSz;
        dataSz -= cfg->eraseSz;
    }

    return WHAL_SUCCESS;
}

#ifndef WHAL_CFG_SPIMEM_NOR_FLASH_DIRECT_API_MAPPING
const whal_FlashDriver whal_SpiMemNor_Flash_Driver = {
    .Init = whal_SpiMemNor_Flash_Init,
    .Deinit = whal_SpiMemNor_Flash_Deinit,
    .Lock = whal_SpiMemNor_Flash_Lock,
    .Unlock = whal_SpiMemNor_Flash_Unlock,
    .Read = whal_SpiMemNor_Flash_Read,
    .Write = whal_SpiMemNor_Flash_Write,
    .Erase = whal_SpiMemNor_Flash_Erase,
};
#endif /* !WHAL_CFG_SPIMEM_NOR_FLASH_DIRECT_API_MAPPING */
