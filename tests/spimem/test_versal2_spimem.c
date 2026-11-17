/* test_versal2_spimem.c
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
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/reg.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * Cadence OSPI controller on the real (or QEMU) registers and flash: one
 * command per whal_SpiMem_Exec, reads by STIG and DMA, the window.
 */

/* The last sector of the flash test area */
#define T_ADDR (BOARD_FLASH_TEST_ADDR + BOARD_FLASH_SECTOR_SZ)
#define T_LEN  1024

/* Controller registers (AM028 OSPI) */
#define T_CONFIG          0x00
#define T_CONFIG_ENB_SPI  (1UL << 0)
#define T_CONFIG_DTR      (1UL << 24)
#define T_CONFIG_DUAL_OP  (1UL << 30)
#define T_OPCODE_EXT_LOW  0xE0

/* OSPI DMA status, write 1 to clear (AM028 OSPIDMA_DST_I_STS) */
#define T_DMA_DST_I_STS   0x1814
#define T_DMA_DONE        (1UL << 1)

static uint8_t g_pattern[T_LEN];
static uint8_t g_buf[T_LEN + 8] __attribute__((aligned(64)));

/* The board controller without its DMA: every read is STIG. */
static whal_CadenceOspi_SpiMem_Cfg g_stigCfg;
static whal_SpiMem g_stigOspi = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = WHAL_VERSAL2_OSPI_DRIVER,
    .cfg = &g_stigCfg,
};

/* The board controller with the real DMA wrapped: one call can be made
 * to fail after (Wait) or instead of (Configure, Start) the real one. */
static whal_Error g_dmaCfgRet;
static whal_Error g_dmaStartRet;
static whal_Error g_dmaWaitRet;
/* 1: Wait sees DONE but does not clear it, like a transfer that ended
 * after an earlier Wait gave up. */
static int g_dmaLeaveDone;
static int g_dmaCfgCalls;

static whal_Error FaultConfigure(whal_Dma *dev, size_t ch, const void *c)
{
    g_dmaCfgCalls++;
    if (g_dmaCfgRet != WHAL_SUCCESS) {
        return g_dmaCfgRet;
    }
    return whal_Versal2_OspiDma_Driver.Configure(dev, ch, c);
}

static whal_Error FaultStart(whal_Dma *dev, size_t ch)
{
    if (g_dmaStartRet != WHAL_SUCCESS) {
        return g_dmaStartRet;
    }
    return whal_Versal2_OspiDma_Driver.Start(dev, ch);
}

static whal_Error FaultWait(whal_Dma *dev, size_t ch)
{
    const whal_Versal2_OspiDma_Cfg *cfg = dev->cfg;
    whal_Error err;

    if (g_dmaLeaveDone) {
        return whal_Reg32_ReadPoll(dev->base, T_DMA_DST_I_STS, T_DMA_DONE,
                                   T_DMA_DONE, cfg->timeout);
    }
    err = whal_Versal2_OspiDma_Driver.Wait(dev, ch);
    if (err == WHAL_SUCCESS) {
        err = g_dmaWaitRet;
    }
    return err;
}

static whal_DmaDriver g_faultDmaDriver;
static whal_Dma g_faultDma = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = &g_faultDmaDriver,
};
static whal_CadenceOspi_SpiMem_Cfg g_faultCfg;
static whal_SpiMem g_faultOspi = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = WHAL_VERSAL2_OSPI_DRIVER,
    .cfg = &g_faultCfg,
};

/* The wrapped DMA again, with a small dmaMaxLen set by the test. */
static whal_CadenceOspi_SpiMem_Cfg g_pieceCfg;
static whal_SpiMem g_pieceOspi = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = WHAL_VERSAL2_OSPI_DRIVER,
    .cfg = &g_pieceCfg,
};

/* The board controller with no window. */
static whal_CadenceOspi_SpiMem_Cfg g_noMapCfg;
static whal_SpiMem g_noMapOspi = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = WHAL_VERSAL2_OSPI_DRIVER,
    .cfg = &g_noMapCfg,
};

static void Setup(void)
{
    const whal_CadenceOspi_SpiMem_Cfg *board = g_whalOspi.cfg;
    size_t i;

    g_stigCfg = *board;
    g_stigCfg.dma = NULL;

    g_faultDmaDriver = whal_Versal2_OspiDma_Driver;
    g_faultDmaDriver.Configure = FaultConfigure;
    g_faultDmaDriver.Start = FaultStart;
    g_faultDmaDriver.Wait = FaultWait;
    g_faultDma.cfg = g_whalOspiDma.cfg;
    g_faultCfg = *board;
    g_faultCfg.dma = &g_faultDma;
    g_pieceCfg = g_faultCfg;

    g_noMapCfg = *board;
    g_noMapCfg.mapBase = 0;

    for (i = 0; i < T_LEN; i++) {
        g_pattern[i] = (uint8_t)((i * 13) ^ (i >> 7) ^ 0x5A);
    }
}

static uint32_t Reg(uint32_t off)
{
    return whal_Reg32_Read(WHAL_VERSAL2_OSPI_BASE, off);
}

/* A 1-bit command with no address; data in or out. */
static whal_Error Cmd(whal_SpiMem *dev, uint8_t opcode, uint8_t dir,
                      uint8_t *data, size_t len)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;

    op.opcode = opcode;
    op.dir = dir;
    op.data = data;
    op.dataSz = len;
    return whal_SpiMem_Exec(dev, &op);
}

/* A 1-bit 4-byte address read of len bytes at T_ADDR + off. */
static whal_Error Read(whal_SpiMem *dev, size_t off, uint8_t *data,
                       size_t len)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;

    op.opcode = BOARD_FLASH_READ4;
    op.addrBytes = 4;
    op.addr = (uint32_t)(T_ADDR + off);
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.data = data;
    op.dataSz = len;
    return whal_SpiMem_Exec(dev, &op);
}

static void Clear(void)
{
    size_t i;

    for (i = 0; i < sizeof(g_buf); i++) {
        g_buf[i] = 0;
    }
}

/* Put the pattern in the test sector, through the board flash. */
static void Test_Versal2SpiMem_Prepare(void)
{
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_whalFlash, T_ADDR,
                                    BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_whalFlash, T_ADDR, g_pattern,
                                    T_LEN), WHAL_SUCCESS);
}

static void Test_Versal2SpiMem_ReadId(void)
{
    uint8_t id[3] = {0, 0, 0};

    WHAL_ASSERT_EQ(Cmd(&g_whalOspi, 0x9F, WHAL_SPIMEM_DIR_IN, id, 3),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(id[0], 0x2C);
    WHAL_ASSERT_EQ(id[1], 0x5B);
    WHAL_ASSERT_EQ(id[2], 0x1C);
}

/* WRITE ENABLE sets the latch in the status register, WRDI clears it. */
static void Test_Versal2SpiMem_WriteEnable(void)
{
    uint8_t sr = 0;

    WHAL_ASSERT_EQ(Cmd(&g_whalOspi, 0x06, WHAL_SPIMEM_DIR_NONE, NULL, 0),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Cmd(&g_whalOspi, 0x05, WHAL_SPIMEM_DIR_IN, &sr, 1),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(sr & 0x03, 0x02);
    WHAL_ASSERT_EQ(Cmd(&g_whalOspi, 0x04, WHAL_SPIMEM_DIR_NONE, NULL, 0),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Cmd(&g_whalOspi, 0x05, WHAL_SPIMEM_DIR_IN, &sr, 1),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(sr & 0x03, 0);
}

/* STIG carries 8 data bytes; a read with an address has a 32-bit count. */
static void Test_Versal2SpiMem_MaxDataSz(void)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    size_t max = 0;

    op.opcode = BOARD_FLASH_PROG4;
    op.addrBytes = 4;
    op.dir = WHAL_SPIMEM_DIR_OUT;
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&g_whalOspi, &op, &max),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(max, 8);
    op.opcode = BOARD_FLASH_READ4;
    op.dir = WHAL_SPIMEM_DIR_IN;
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&g_whalOspi, &op, &max),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(max == UINT32_MAX, 1);
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&g_whalOspi, &op, NULL),
                   WHAL_EINVAL);
}

/* Bad ops are refused before any register is touched. */
static void Test_Versal2SpiMem_BadOps(void)
{
    whal_SpiMemOp good = WHAL_SPIMEM_OP_INIT_1BIT;
    whal_SpiMemOp op;
    uint8_t b[2];

    good.opcode = 0x05;
    good.dir = WHAL_SPIMEM_DIR_IN;
    good.data = b;
    good.dataSz = 1;

    op = good;
    op.opLines = 3;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EINVAL);
    op = good;
    op.opBytes = 3;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EINVAL);
    op = good;
    op.addrBytes = 5;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EINVAL);
    op = good;
    op.dummyCycles = 32;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EINVAL);
    op = good;
    op.dir = 3;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EINVAL);
    op = good;
    op.dir = WHAL_SPIMEM_DIR_NONE;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EINVAL);
    op = good;
    op.data = NULL;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EINVAL);
    op = good;
    op.dtr = 1;
    op.dataSz = 1;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EINVAL);
    /* A read may not run past address FFFFFFFFh or count over 32 bits */
    op = good;
    op.opcode = BOARD_FLASH_READ4;
    op.addrBytes = 4;
    op.addr = 0xFFFFFFFF;
    op.dataSz = 2;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EINVAL);
    op.addr = 0;
    op.dataSz = 0x100000000ULL;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EINVAL);
    /* AM028: dual-byte opcode mode is octal only */
    op = good;
    op.opBytes = 2;
    op.opcode = 0x0505;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, NULL), WHAL_EINVAL);
}

/* A 2-byte octal DTR op (the octal reset) sets dual-byte mode, DTR and
 * the second opcode byte; the next 1-bit op turns both modes off. */
static void Test_Versal2SpiMem_DualByteOp(void)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    uint8_t id[3] = {0, 0, 0};

    op.opLines = WHAL_SPIMEM_LINES_8;
    op.dtr = 1;
    op.opBytes = 2;
    op.dir = WHAL_SPIMEM_DIR_NONE;
    op.opcode = 0x6666;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_SUCCESS);
    op.opcode = 0x9999;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(T_CONFIG) & (T_CONFIG_DTR | T_CONFIG_DUAL_OP),
                   T_CONFIG_DTR | T_CONFIG_DUAL_OP);
    WHAL_ASSERT_EQ(Reg(T_OPCODE_EXT_LOW) & 0xFF, 0x99);
    Board_WaitMs(1);

    WHAL_ASSERT_EQ(Cmd(&g_whalOspi, 0x9F, WHAL_SPIMEM_DIR_IN, id, 3),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(T_CONFIG) & (T_CONFIG_DTR | T_CONFIG_DUAL_OP), 0);
    WHAL_ASSERT_EQ(id[0], 0x2C);
}

/* Reads of many starts and lengths, by DMA (board) and by STIG only,
 * against the same flash data. */
static void Test_Versal2SpiMem_ReadSizes(void)
{
    static const size_t lens[] = {
        1, 3, 4, 5, 7, 8, 9, 15, 16, 17, 63, 64, 65, 200, 1000
    };
    size_t o;
    size_t i;

    for (o = 0; o < 5; o++) {
        for (i = 0; i < sizeof(lens) / sizeof(lens[0]); i++) {
            if (o + lens[i] > T_LEN) {
                continue;
            }
            Clear();
            WHAL_ASSERT_EQ(Read(&g_whalOspi, o, &g_buf[o], lens[i]),
                           WHAL_SUCCESS);
            WHAL_ASSERT_MEM_EQ(&g_buf[o], &g_pattern[o], lens[i]);
            Clear();
            WHAL_ASSERT_EQ(Read(&g_stigOspi, o, &g_buf[o], lens[i]),
                           WHAL_SUCCESS);
            WHAL_ASSERT_MEM_EQ(&g_buf[o], &g_pattern[o], lens[i]);
        }
    }
}

/* After a failed DMA read the controller must be usable: the indirect
 * read was cancelled and the next DMA read returns the right data. */
static void CheckReadAfterFault(void)
{
    Clear();
    WHAL_ASSERT_EQ(Read(&g_whalOspi, 0, g_buf, 256), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(g_buf, g_pattern, 256);
}

/* QEMU's DMA model reports a write to an unmapped address with
 * DST_I_STS AXI_BRESP_ERR (AM026 Table 225, bit 2). QEMU only. */
static void Test_Versal2SpiMem_DmaBusError(void)
{
#ifndef BOARD_QEMU
    WHAL_SKIP();
#else
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;

    op.opcode = BOARD_FLASH_READ4;
    op.addrBytes = 4;
    op.addr = (uint32_t)T_ADDR;
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.data = (void *)(uintptr_t)0x90000000UL;
    op.dataSz = 256;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&g_whalOspi, &op), WHAL_EHARDWARE);
    CheckReadAfterFault();
#endif
}

/* A DMA error reported at the end of the transfer fails the read. */
static void Test_Versal2SpiMem_DmaWaitFails(void)
{
    g_dmaWaitRet = WHAL_EHARDWARE;
    WHAL_ASSERT_EQ(Read(&g_faultOspi, 0, g_buf, 256), WHAL_EHARDWARE);
    g_dmaWaitRet = WHAL_SUCCESS;
    CheckReadAfterFault();
}

/* Configure or Start failing: the read fails before it starts. */
static void Test_Versal2SpiMem_DmaSetupFails(void)
{
    g_dmaCfgRet = WHAL_EINVAL;
    WHAL_ASSERT_EQ(Read(&g_faultOspi, 0, g_buf, 256), WHAL_EINVAL);
    g_dmaCfgRet = WHAL_SUCCESS;
    CheckReadAfterFault();
    g_dmaStartRet = WHAL_EINVAL;
    WHAL_ASSERT_EQ(Read(&g_faultOspi, 0, g_buf, 256), WHAL_EINVAL);
    g_dmaStartRet = WHAL_SUCCESS;
    CheckReadAfterFault();
}

/* A DONE bit left by an earlier transfer must not end the next Wait:
 * Configure clears it (AM028 OSPIDMA_DST_I_STS, write 1 to clear). */
static void Test_Versal2SpiMem_DmaStaleDone(void)
{
    whal_DmaMemXfer x = { g_buf, 64 };
    whal_Error err;

    g_dmaLeaveDone = 1;
    err = Read(&g_faultOspi, 0, g_buf, 256);
    g_dmaLeaveDone = 0;
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(T_DMA_DST_I_STS) & T_DMA_DONE, T_DMA_DONE);
    WHAL_ASSERT_EQ(whal_Dma_Configure(&g_whalOspiDma,
                                      WHAL_VERSAL2_OSPI_DMA_CH, &x),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(T_DMA_DST_I_STS) & T_DMA_DONE, 0);
    CheckReadAfterFault();
}

/* One read of len bytes at off with dmaMaxLen max: the data is right and
 * the DMA was set up calls times. */
static void CheckPieces(size_t max, size_t off, size_t len, int calls)
{
    g_pieceCfg.dmaMaxLen = max;
    g_dmaCfgCalls = 0;
    Clear();
    WHAL_ASSERT_EQ(Read(&g_pieceOspi, off, &g_buf[off], len), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_dmaCfgCalls, calls);
    WHAL_ASSERT_MEM_EQ(&g_buf[off], &g_pattern[off], len);
}

/* A DMA part longer than dmaMaxLen goes as several transfers, each a
 * multiple of the 4-byte alignment. Too small a limit: STIG reads it. */
static void Test_Versal2SpiMem_DmaPieces(void)
{
    /* 256 in one; 4 x 64; 100 + 100 + 56 (101 rounds down to 100) */
    CheckPieces(256, 0, 256, 1);
    CheckPieces(64, 0, 256, 4);
    CheckPieces(101, 0, 256, 3);
    /* 3 by STIG, then 196 = 3 x 64 + 4 by DMA, then 1 by STIG */
    CheckPieces(64, 1, 200, 4);
    CheckPieces(2, 0, 64, 0);
}

static void MapInit(whal_SpiMemMap *map)
{
    whal_SpiMemOp tmpl = WHAL_SPIMEM_OP_INIT_1BIT;

    tmpl.opcode = BOARD_FLASH_READ4;
    tmpl.addrBytes = 4;
    map->tmpl = tmpl;
    map->offset = T_ADDR;
    map->length = T_LEN;
}

/* Window reads of many starts and lengths. */
static void Test_Versal2SpiMem_MapReads(void)
{
    static const size_t lens[] = { 1, 2, 3, 4, 5, 7, 9, 64, 1000 };
    whal_SpiMemMap map;
    size_t o;
    size_t i;

    MapInit(&map);
    WHAL_ASSERT_EQ(whal_SpiMem_MapCreate(&g_whalOspi, &map), WHAL_SUCCESS);
    for (o = 0; o < 4; o++) {
        for (i = 0; i < sizeof(lens) / sizeof(lens[0]); i++) {
            Clear();
            WHAL_ASSERT_EQ(whal_SpiMem_MapRead(&g_whalOspi, &map, o,
                                               g_buf, lens[i]),
                           WHAL_SUCCESS);
            WHAL_ASSERT_MEM_EQ(g_buf, &g_pattern[o], lens[i]);
        }
    }
    WHAL_ASSERT_EQ(whal_SpiMem_MapDestroy(&g_whalOspi, &map), WHAL_SUCCESS);
}

static void Test_Versal2SpiMem_MapErrors(void)
{
    whal_SpiMemMap map;
    whal_SpiMemMap bad;

    MapInit(&map);
    WHAL_ASSERT_EQ(whal_SpiMem_MapCreate(&g_noMapOspi, &map), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_SpiMem_MapRead(&g_noMapOspi, &map, 0, g_buf, 1),
                   WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_SpiMem_MapCreate(&g_whalOspi, NULL), WHAL_EINVAL);

    /* Single rate only */
    bad = map;
    bad.tmpl.dtr = 1;
    WHAL_ASSERT_EQ(whal_SpiMem_MapCreate(&g_whalOspi, &bad), WHAL_ENOTSUP);
    /* The window sends the controller's 4 address bytes */
    bad = map;
    bad.tmpl.addrBytes = 3;
    WHAL_ASSERT_EQ(whal_SpiMem_MapCreate(&g_whalOspi, &bad), WHAL_EINVAL);
    bad = map;
    bad.length = 0;
    WHAL_ASSERT_EQ(whal_SpiMem_MapCreate(&g_whalOspi, &bad), WHAL_EINVAL);
    bad = map;
    bad.offset = WHAL_VERSAL2_OSPI_MAP_SIZE;
    WHAL_ASSERT_EQ(whal_SpiMem_MapCreate(&g_whalOspi, &bad), WHAL_EINVAL);
    bad = map;
    bad.tmpl.opBytes = 2;
    WHAL_ASSERT_EQ(whal_SpiMem_MapCreate(&g_whalOspi, &bad), WHAL_ENOTSUP);

    /* Reads must stay inside the map */
    WHAL_ASSERT_EQ(whal_SpiMem_MapCreate(&g_whalOspi, &map), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_SpiMem_MapRead(&g_whalOspi, &map, T_LEN, g_buf, 1),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMem_MapRead(&g_whalOspi, &map, 0, NULL, 1),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMem_MapDestroy(&g_whalOspi, &map), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_SpiMem_MapDestroy(&g_whalOspi, NULL), WHAL_EINVAL);
}

/* Deinit turns the controller off; Init turns it back on. */
static void Test_Versal2SpiMem_DeinitInit(void)
{
    uint8_t id[3] = {0, 0, 0};

    WHAL_ASSERT_EQ(whal_SpiMem_Deinit(&g_whalOspi), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(T_CONFIG) & T_CONFIG_ENB_SPI, 0);
    WHAL_ASSERT_EQ(whal_SpiMem_Init(&g_whalOspi), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Reg(T_CONFIG) & T_CONFIG_ENB_SPI, T_CONFIG_ENB_SPI);
    WHAL_ASSERT_EQ(Cmd(&g_whalOspi, 0x9F, WHAL_SPIMEM_DIR_IN, id, 3),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(id[0], 0x2C);
    WHAL_ASSERT_EQ(whal_SpiMem_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMem_Deinit(NULL), WHAL_EINVAL);
}

static void Test_Versal2SpiMem_Cleanup(void)
{
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_whalFlash, T_ADDR,
                                    BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
}

void whal_Test_SpiMem_Platform(void)
{
    Setup();

    WHAL_TEST_SUITE_START("spimem_platform");
    WHAL_TEST(Test_Versal2SpiMem_Prepare);
    WHAL_TEST(Test_Versal2SpiMem_ReadId);
    WHAL_TEST(Test_Versal2SpiMem_WriteEnable);
    WHAL_TEST(Test_Versal2SpiMem_MaxDataSz);
    WHAL_TEST(Test_Versal2SpiMem_BadOps);
    WHAL_TEST(Test_Versal2SpiMem_DualByteOp);
    WHAL_TEST(Test_Versal2SpiMem_ReadSizes);
    WHAL_TEST(Test_Versal2SpiMem_DmaBusError);
    WHAL_TEST(Test_Versal2SpiMem_DmaWaitFails);
    WHAL_TEST(Test_Versal2SpiMem_DmaSetupFails);
    WHAL_TEST(Test_Versal2SpiMem_DmaStaleDone);
    WHAL_TEST(Test_Versal2SpiMem_DmaPieces);
    WHAL_TEST(Test_Versal2SpiMem_MapReads);
    WHAL_TEST(Test_Versal2SpiMem_MapErrors);
    WHAL_TEST(Test_Versal2SpiMem_DeinitInit);
    WHAL_TEST(Test_Versal2SpiMem_Cleanup);
    WHAL_TEST_SUITE_END();
}
