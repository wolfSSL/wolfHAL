/* test_versal2_flash.c
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

#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/reg.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * OSPI NOR flash tests: the same data read through the board flash (DMA
 * for the aligned middle of a read) and through a STIG-only controller.
 */

#define TEST_ADDR BOARD_FLASH_TEST_ADDR
#define TEST_LEN  4096

static uint8_t g_pattern[TEST_LEN];
static uint8_t g_readback[TEST_LEN] __attribute__((aligned(64)));

/* The board's OSPI DMA, counting the transfers it starts. */
static int g_dmaArms;
static whal_DmaDriver g_countDmaDriver;
static whal_Dma g_countDma = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = &g_countDmaDriver,
};

static whal_Error CountStart(whal_Dma *dmaDev, size_t ch)
{
    g_dmaArms++;
    return whal_Versal2_OspiDma_Driver.Start(dmaDev, ch);
}

/* The board controller and flash, with the counting DMA. */
static whal_CadenceOspi_SpiMem_Cfg g_dmaOspiCfg;
static whal_SpiMem g_dmaOspi = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = WHAL_VERSAL2_OSPI_DRIVER,
    .cfg = &g_dmaOspiCfg,
};
static whal_SpiMemNor_Flash_Cfg g_dmaFlashCfg;
static whal_Flash g_dmaFlash = {
    .driver = &whal_SpiMemNor_Flash_Driver,
    .cfg = &g_dmaFlashCfg,
};

/* The board flash, reading through the memory-mapped window. */
static whal_SpiMemNor_Flash_Cfg g_mapFlashCfg;
static whal_Flash g_mapFlash = {
    .driver = &whal_SpiMemNor_Flash_Driver,
    .cfg = &g_mapFlashCfg,
};

/* The same, with no DMA: every read is STIG. */
static whal_CadenceOspi_SpiMem_Cfg g_stigOspiCfg;
static whal_SpiMem g_stigOspi = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = WHAL_VERSAL2_OSPI_DRIVER,
    .cfg = &g_stigOspiCfg,
};
static whal_SpiMemNor_Flash_Cfg g_stigFlashCfg;
static whal_Flash g_stigFlash = {
    .driver = &whal_SpiMemNor_Flash_Driver,
    .cfg = &g_stigFlashCfg,
};

static void Setup(void)
{
    const whal_CadenceOspi_SpiMem_Cfg *ospi = g_whalOspi.cfg;
    const whal_SpiMemNor_Flash_Cfg *flash = g_whalFlash.cfg;
    size_t i;

    g_countDmaDriver = whal_Versal2_OspiDma_Driver;
    g_countDmaDriver.Start = CountStart;
    g_countDma.cfg = g_whalOspiDma.cfg;

    g_dmaOspiCfg = *ospi;
    g_dmaOspiCfg.dma = &g_countDma;
    g_stigOspiCfg = *ospi;
    g_stigOspiCfg.dma = NULL;

    g_dmaFlashCfg = *flash;
    g_dmaFlashCfg.spiMemDev = &g_dmaOspi;
    g_stigFlashCfg = *flash;
    g_stigFlashCfg.spiMemDev = &g_stigOspi;
    g_mapFlashCfg = g_dmaFlashCfg;
    g_mapFlashCfg.flags |= WHAL_SPIMEM_NOR_FLAG_MAP_READ;

    for (i = 0; i < TEST_LEN; i++) {
        g_pattern[i] = (uint8_t)((i * 7) ^ (i >> 8));
    }
}

static void ClearReadback(void)
{
    size_t i;

    for (i = 0; i < TEST_LEN; i++) {
        g_readback[i] = 0;
    }
}

static void Test_Versal2Flash_Id(void)
{
    uint8_t id[3] = {0, 0, 0};

    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_ReadId(&g_whalFlash, id),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(id[0], 0x2C);
    WHAL_ASSERT_EQ(id[1], 0x5B);
    WHAL_ASSERT_EQ(id[2], 0x1C);
}

static void Test_Versal2Flash_Program(void)
{
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_dmaFlash), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_dmaFlash, TEST_ADDR,
                                    BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_dmaFlash, TEST_ADDR, g_pattern,
                                    TEST_LEN), WHAL_SUCCESS);
}

static void Test_Versal2Flash_DmaRead(void)
{
    ClearReadback();
    g_dmaArms = 0;
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_dmaFlash, TEST_ADDR, g_readback,
                                   TEST_LEN), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(g_readback, g_pattern, TEST_LEN);
    WHAL_ASSERT_NEQ(g_dmaArms, 0);
}

static void Test_Versal2Flash_StigRead(void)
{
    ClearReadback();
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_stigFlash), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_stigFlash, TEST_ADDR, g_readback,
                                   TEST_LEN), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(g_readback, g_pattern, TEST_LEN);
}

/* An odd start and length: STIG head and tail, DMA in the middle. The
 * buffer and flash address are off by the same amount, so the middle
 * meets the DMA's 4-byte alignment on both. */
static void Test_Versal2Flash_UnalignedRead(void)
{
    const size_t off = 3;
    const size_t len = 1001;

    ClearReadback();
    g_dmaArms = 0;
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_dmaFlash, TEST_ADDR + off,
                                   &g_readback[off], len), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(&g_readback[off], &g_pattern[off], len);
    WHAL_ASSERT_NEQ(g_dmaArms, 0);
}

/* Buffer and flash address off by different amounts: no DMA is possible,
 * so the whole read is STIG and still correct. */
static void Test_Versal2Flash_MisalignedRead(void)
{
    const size_t off = 3;
    const size_t len = 1001;

    ClearReadback();
    g_dmaArms = 0;
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_dmaFlash, TEST_ADDR + off,
                                   &g_readback[1], len), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(&g_readback[1], &g_pattern[off], len);
    WHAL_ASSERT_EQ(g_dmaArms, 0);
}

static uint32_t AxiSel(void)
{
    return whal_Reg32_Read(WHAL_VERSAL2_OSPI_AXI_SEL_ADDR, 0);
}

/* Through the window (direct mode), then the AXI port goes back to DMA. */
static void Test_Versal2Flash_MapRead(void)
{
    ClearReadback();
    g_dmaArms = 0;
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_mapFlash, TEST_ADDR, g_readback,
                                   TEST_LEN), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(g_readback, g_pattern, TEST_LEN);
    WHAL_ASSERT_EQ(g_dmaArms, 0);
    WHAL_ASSERT_EQ(AxiSel() & WHAL_VERSAL2_OSPI_MUX_SEL_Msk, 0);
}

/* A map left open routes the port to the window; closing it routes it
 * back, and DMA reads work again. */
static void Test_Versal2Flash_DmaAfterMap(void)
{
    whal_SpiMemMap map;
    whal_SpiMemOp tmpl = WHAL_SPIMEM_OP_INIT_1BIT;
    uint8_t b = 0;

    tmpl.opcode = BOARD_FLASH_READ4;
    tmpl.addrBytes = 4;
    map.tmpl = tmpl;
    map.offset = TEST_ADDR;
    map.length = TEST_LEN;
    WHAL_ASSERT_EQ(whal_SpiMem_MapCreate(&g_dmaOspi, &map), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(AxiSel() & WHAL_VERSAL2_OSPI_MUX_SEL_Msk,
                   WHAL_VERSAL2_OSPI_MUX_SEL_Msk);
    WHAL_ASSERT_EQ(whal_SpiMem_MapRead(&g_dmaOspi, &map, 5, &b, 1),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(b, g_pattern[5]);
    WHAL_ASSERT_EQ(whal_SpiMem_MapDestroy(&g_dmaOspi, &map), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(AxiSel() & WHAL_VERSAL2_OSPI_MUX_SEL_Msk, 0);

    ClearReadback();
    g_dmaArms = 0;
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_dmaFlash, TEST_ADDR, g_readback,
                                   TEST_LEN), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(g_readback, g_pattern, TEST_LEN);
    WHAL_ASSERT_NEQ(g_dmaArms, 0);
}

static void Test_Versal2Flash_Erase(void)
{
    size_t i;

    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_dmaFlash, TEST_ADDR,
                                    BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
    ClearReadback();
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_stigFlash, TEST_ADDR, g_readback,
                                   TEST_LEN), WHAL_SUCCESS);
    for (i = 0; i < TEST_LEN; i++) {
        WHAL_ASSERT_EQ(g_readback[i], 0xFF);
    }
}

/* Init refuses another part, also after a reset. */
static void Test_Versal2Flash_InitWrongId(void)
{
    whal_SpiMemNor_Flash_Cfg cfg = g_dmaFlashCfg;
    whal_Flash dev = { .driver = &whal_SpiMemNor_Flash_Driver, .cfg = &cfg };

    cfg.id = 0x1C5A2C;
    cfg.flags = 0;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_EHARDWARE);
    cfg.flags = WHAL_SPIMEM_NOR_FLAG_RESET;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_dmaFlash), WHAL_SUCCESS);
}

/* Bad sizes are refused before any command is sent. */
static void Test_Versal2Flash_InitBadCfg(void)
{
    whal_SpiMemNor_Flash_Cfg cfg;
    whal_Flash dev = { .driver = &whal_SpiMemNor_Flash_Driver, .cfg = &cfg };

    cfg = g_dmaFlashCfg;
    cfg.pageSz = 100;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_EINVAL);
    cfg = g_dmaFlashCfg;
    cfg.eraseSz = 0;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_EINVAL);
    cfg = g_dmaFlashCfg;
    cfg.capacity = 0;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_EINVAL);
    cfg = g_dmaFlashCfg;
    cfg.addrBytes = 2;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_EINVAL);
    /* The capacity must fit the address bytes: 16 MB for 3, 4 GB for 4 */
    cfg = g_dmaFlashCfg;
    cfg.addrBytes = 3;
    cfg.capacity = 0x1000001;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_EINVAL);
    cfg.capacity = 0x1000000;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_SUCCESS);
    cfg = g_dmaFlashCfg;
    cfg.capacity = 0x100000001ULL;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_EINVAL);
    cfg = g_dmaFlashCfg;
    cfg.rstRegAddrBytes = 2;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Init(NULL), WHAL_EINVAL);
}

/* A write that starts mid-page and crosses page ends; the bytes around
 * it stay erased. */
static void Test_Versal2Flash_WriteAcrossPages(void)
{
    const size_t off = 250;
    const size_t len = 600;

    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_dmaFlash, TEST_ADDR,
                                    BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_dmaFlash, TEST_ADDR + off, g_pattern,
                                    len), WHAL_SUCCESS);
    ClearReadback();
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_dmaFlash, TEST_ADDR, g_readback,
                                   off + len + 4), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(&g_readback[off], g_pattern, len);
    WHAL_ASSERT_EQ(g_readback[off - 1], 0xFF);
    WHAL_ASSERT_EQ(g_readback[off + len], 0xFF);
}

/* Erase takes whole sectors inside the flash only, and erases nothing
 * when it refuses. */
static void Test_Versal2Flash_EraseErrors(void)
{
    const size_t sz = BOARD_FLASH_SECTOR_SZ;
    uint8_t b = 0;

    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_dmaFlash, TEST_ADDR + 4096, sz),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_dmaFlash, TEST_ADDR, sz + 1),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_dmaFlash, BOARD_FLASH_SIZE, sz),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_dmaFlash, TEST_ADDR, 0),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_dmaFlash, TEST_ADDR + 250, &b, 1),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(b, g_pattern[0]);
}

static void Test_Versal2Flash_Bounds(void)
{
    uint8_t b[4];

    WHAL_ASSERT_EQ(whal_Flash_Read(&g_dmaFlash, BOARD_FLASH_SIZE - 2, b, 4),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_dmaFlash, BOARD_FLASH_SIZE, b, 1),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Read(&g_dmaFlash, 0, NULL, 4),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Write(&g_dmaFlash, 0, NULL, 4),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Read(&g_dmaFlash, 0, b, 0),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Write(&g_dmaFlash, 0, b, 0),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_ReadId(&g_dmaFlash, NULL),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_ReadId(NULL, b), WHAL_EINVAL);
}

/* No protect commands: Lock and Unlock are not supported. */
static void Test_Versal2Flash_LockUnlock(void)
{
    WHAL_ASSERT_EQ(whal_Flash_Lock(&g_dmaFlash, TEST_ADDR, TEST_LEN),
                   WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Flash_Unlock(&g_dmaFlash, TEST_ADDR, TEST_LEN),
                   WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Lock(NULL, 0, 0), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Unlock(NULL, 0, 0), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Deinit(&g_dmaFlash), WHAL_SUCCESS);
}

/* With the per-part values off the driver sends only common commands,
 * and program and erase still work. */
static void Test_Versal2Flash_NoPartValues(void)
{
    whal_SpiMemNor_Flash_Cfg cfg = g_dmaFlashCfg;
    whal_Flash dev = { .driver = &whal_SpiMemNor_Flash_Driver, .cfg = &cfg };

    cfg.errOp = 0;
    cfg.errClearOp = 0;
    cfg.rstRegOp = 0;
    WHAL_ASSERT_EQ(whal_Flash_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&dev, TEST_ADDR, BOARD_FLASH_SECTOR_SZ),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Write(&dev, TEST_ADDR, g_pattern, 64),
                   WHAL_SUCCESS);
    ClearReadback();
    WHAL_ASSERT_EQ(whal_Flash_Read(&dev, TEST_ADDR, g_readback, 64),
                   WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(g_readback, g_pattern, 64);
}

static int FlashIdOk(void)
{
    uint8_t id[3] = {0, 0, 0};

    (void)whal_SpiMemNor_Flash_ReadId(&g_dmaFlash, id);
    if (id[0] == 0x2C && id[1] == 0x5B && id[2] == 0x1C) {
        return 1;
    }
    return 0;
}

/* Volatile config I/O mode (81h at 00h) = E7h: octal DDR. Address bytes
 * as the flag status register (70h) bit 0 says. */
static whal_Error EnterOctal(void)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    uint8_t fsr = 0;
    uint8_t val = 0xE7;
    whal_Error err;

    op.opcode = 0x70;
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.data = &fsr;
    op.dataSz = 1;
    err = whal_SpiMem_Exec(&g_whalOspi, &op);
    if (err) {
        return err;
    }
    op.opcode = 0x06;
    op.dir = WHAL_SPIMEM_DIR_NONE;
    op.data = NULL;
    op.dataSz = 0;
    err = whal_SpiMem_Exec(&g_whalOspi, &op);
    if (err) {
        return err;
    }
    op.opcode = 0x81;
    op.addrBytes = 3;
    if (fsr & 1) {
        op.addrBytes = 4;
    }
    op.addr = 0;
    op.dir = WHAL_SPIMEM_DIR_OUT;
    op.data = &val;
    op.dataSz = 1;
    return whal_SpiMem_Exec(&g_whalOspi, &op);
}

/* 66h, 99h in 1-bit mode, then wait for not busy */
static whal_Error ResetFlash(void)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    whal_Error err;

    op.dir = WHAL_SPIMEM_DIR_NONE;
    op.opcode = 0x66;
    err = whal_SpiMem_Exec(&g_whalOspi, &op);
    if (err) {
        return err;
    }
    op.opcode = 0x99;
    err = whal_SpiMem_Exec(&g_whalOspi, &op);
    Board_WaitMs(1);
    return err;
}

/* A flash left in octal DDR: without RESET_OCTAL_DTR Init can't reach
 * it; the board flags bring it back. QEMU's flash has no octal mode. */
static void Test_Versal2Flash_OctalRecovery(void)
{
    whal_SpiMemNor_Flash_Cfg cfg = g_dmaFlashCfg;
    whal_Flash dev = { .driver = &whal_SpiMemNor_Flash_Driver, .cfg = &cfg };
    whal_Timeout timeout;
    whal_Error err;

    WHAL_ASSERT_EQ(EnterOctal(), WHAL_SUCCESS);
    Board_WaitMs(1);
    if (FlashIdOk()) {
        /* Not in octal (QEMU): reset to reload the volatile config */
        (void)ResetFlash();
        WHAL_SKIP();
    }
    /* The 1-bit reset isn't heard and status reads give FFh (busy), so
     * this Init fails by ID or by timeout. */
    timeout = g_whalTimeout;
    timeout.timeoutTicks = 50;
    cfg.timeout = &timeout;
    cfg.flags = WHAL_SPIMEM_NOR_FLAG_RESET;
    err = whal_Flash_Init(&dev);
    /* Recover before checking, so a failure can't leave it in octal */
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_dmaFlash), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(err == WHAL_EHARDWARE || err == WHAL_ETIMEOUT, 1);
    WHAL_ASSERT_EQ(FlashIdOk(), 1);
}

/*
 * Fault injection. Every command runs on the real (or QEMU) controller and
 * flash; the wrapper below only changes these values in the replies.
 */

/* Flag status register (70h) errors, set until 50h: MT35XU02G datasheet
 * Table 5; Linux micron-st.c FSR_E_ERR, FSR_P_ERR, FSR_PT_ERR. */
#define T_FSR_ERASE_ERR 0x20
#define T_FSR_PROG_ERR  0x10
#define T_FSR_PROT_ERR  0x02

/* Status register (05h): MT35XU02G Table 3; Linux SR_WIP, SR_WEL. */
#define T_SR_WIP 0x01
#define T_SR_WEL 0x02

static uint8_t g_fsrErr;   /* error bits 70h reports until 50h */
static int g_stuckBusy;    /* 05h always reports WIP */
static int g_noWel;        /* 05h never reports WEL */
static int g_execs;        /* commands seen */
static int g_failAt;       /* command number that fails, 0 = none */
static int g_progSent;     /* program commands sent */

static whal_Error FaultExec(whal_SpiMem *dev, const whal_SpiMemOp *op)
{
    uint8_t *data = op->data;
    whal_Error err;

    g_execs++;
    if (g_failAt != 0 && g_execs == g_failAt) {
        return WHAL_EHARDWARE;
    }
    if (op->opcode == BOARD_FLASH_PROG4) {
        g_progSent++;
    }
    err = whal_CadenceOspi_SpiMem_Driver.Exec(dev, op);
    if (err != WHAL_SUCCESS || op->dir != WHAL_SPIMEM_DIR_IN) {
        if (op->opcode == 0x50) {
            g_fsrErr = 0;
        }
        return err;
    }
    if (op->opcode == 0x70) {
        data[0] |= g_fsrErr;
    }
    if (op->opcode == 0x05 && g_stuckBusy) {
        data[0] |= T_SR_WIP;
    }
    if (op->opcode == 0x05 && g_noWel) {
        data[0] &= (uint8_t)~T_SR_WEL;
    }
    return WHAL_SUCCESS;
}

static whal_SpiMemDriver g_faultSpiMemDriver;
static whal_SpiMem g_faultOspi = {
    .base = WHAL_VERSAL2_OSPI_BASE,
    .driver = &g_faultSpiMemDriver,
};
static whal_Timeout g_shortTimeout;
static whal_SpiMemNor_Flash_Cfg g_faultFlashCfg;
static whal_Flash g_faultFlash = {
    .driver = &whal_SpiMemNor_Flash_Driver,
    .cfg = &g_faultFlashCfg,
};

static void FaultReset(void)
{
    g_faultSpiMemDriver = whal_CadenceOspi_SpiMem_Driver;
    g_faultSpiMemDriver.Exec = FaultExec;
    g_faultOspi.cfg = g_whalOspi.cfg;
    g_shortTimeout = g_whalTimeout;
    g_shortTimeout.timeoutTicks = 50;
    g_faultFlashCfg = g_stigFlashCfg;
    g_faultFlashCfg.spiMemDev = &g_faultOspi;
    g_fsrErr = 0;
    g_stuckBusy = 0;
    g_noWel = 0;
    g_execs = 0;
    g_failAt = 0;
    g_progSent = 0;
}

/* A program or protection error fails the write and is cleared, so the
 * next write works. */
static void Test_Versal2Flash_ProgramError(void)
{
    FaultReset();
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_faultFlash, TEST_ADDR,
                                    BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
    g_fsrErr = T_FSR_PROG_ERR;
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_faultFlash, TEST_ADDR, g_pattern, 16),
                   WHAL_EHARDWARE);
    /* It stops after the first piece and clears the error */
    WHAL_ASSERT_EQ(g_progSent, 1);
    WHAL_ASSERT_EQ(g_fsrErr, 0);
    g_fsrErr = T_FSR_PROT_ERR;
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_faultFlash, TEST_ADDR, g_pattern, 4),
                   WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(g_fsrErr, 0);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_faultFlash, TEST_ADDR + 256,
                                    g_pattern, 16), WHAL_SUCCESS);
}

static void Test_Versal2Flash_EraseError(void)
{
    FaultReset();
    g_fsrErr = T_FSR_ERASE_ERR;
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_faultFlash, TEST_ADDR,
                                    BOARD_FLASH_SECTOR_SZ), WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(g_fsrErr, 0);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_faultFlash, TEST_ADDR,
                                    BOARD_FLASH_SECTOR_SZ), WHAL_SUCCESS);
}

/* With no errClearOp the error still fails the write; nothing clears it. */
static void Test_Versal2Flash_ErrorNoClearOp(void)
{
    FaultReset();
    g_faultFlashCfg.errClearOp = 0;
    g_fsrErr = T_FSR_PROG_ERR;
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_faultFlash, TEST_ADDR + 512,
                                    g_pattern, 4), WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(g_fsrErr, T_FSR_PROG_ERR);
}

/* A flash that stays busy, or never sets the write enable latch, times
 * out; with no latch no program is sent. */
static void Test_Versal2Flash_Timeouts(void)
{
#ifdef WHAL_CFG_NO_TIMEOUT
    WHAL_SKIP();
#else
    FaultReset();
    g_faultFlashCfg.timeout = &g_shortTimeout;
    g_noWel = 1;
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_faultFlash, TEST_ADDR + 768,
                                    g_pattern, 4), WHAL_ETIMEOUT);
    WHAL_ASSERT_EQ(g_progSent, 0);
    g_noWel = 0;
    g_stuckBusy = 1;
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_faultFlash, TEST_ADDR + 768,
                                    g_pattern, 4), WHAL_ETIMEOUT);
    g_stuckBusy = 0;
#endif
}

/* Wait for the real flash to finish (a failed run can leave an erase or
 * program going): status (05h) WIP clear, up to 5 s. */
static void WaitFlashIdle(void)
{
    uint8_t sr = T_SR_WIP;
    int ms;
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;

    op.opcode = 0x05;
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.data = &sr;
    op.dataSz = 1;
    for (ms = 0; ms < 5000; ms++) {
        if (whal_SpiMem_Exec(&g_whalOspi, &op) != WHAL_SUCCESS ||
            (sr & T_SR_WIP) == 0) {
            return;
        }
        Board_WaitMs(1);
    }
}

/* Run op once and count its commands, then make each of the first 16 fail
 * in turn: the error comes back and no command is sent after it. A run
 * may finish first (status polls vary on the board). */
#define FAIL_EACH_MAX 16
static int FailEach(whal_Error (*op)(void))
{
    int total;
    int n;
    whal_Error err;

    FaultReset();
    if (op() != WHAL_SUCCESS) {
        return 0;
    }
    total = g_execs;
    if (total > FAIL_EACH_MAX) {
        total = FAIL_EACH_MAX;
    }
    for (n = 1; n <= total; n++) {
        WaitFlashIdle();
        FaultReset();
        g_failAt = n;
        err = op();
        if (err == WHAL_SUCCESS && g_execs < n) {
            break;
        }
        if (err != WHAL_EHARDWARE || g_execs != n) {
            whal_Test_Printf("  command %d of %d: result %d, %d sent\n",
                             n, total, (int)err, g_execs);
            return 0;
        }
    }
    WaitFlashIdle();
    return total;
}

static whal_Error OpInit(void)
{
    return whal_Flash_Init(&g_faultFlash);
}

static whal_Error OpErase(void)
{
    return whal_Flash_Erase(&g_faultFlash, TEST_ADDR, BOARD_FLASH_SECTOR_SZ);
}

static whal_Error OpWrite(void)
{
    return whal_Flash_Write(&g_faultFlash, TEST_ADDR + 1000, g_pattern, 20);
}

static whal_Error OpRead(void)
{
    return whal_Flash_Read(&g_faultFlash, TEST_ADDR, g_readback, 20);
}

static void Test_Versal2Flash_FailEachCommand(void)
{
    WHAL_ASSERT_NEQ(FailEach(OpInit), 0);
    WHAL_ASSERT_NEQ(FailEach(OpErase), 0);
    WHAL_ASSERT_NEQ(FailEach(OpWrite), 0);
    WHAL_ASSERT_NEQ(FailEach(OpRead), 0);
}

void whal_Test_Flash_Platform(void)
{
    Setup();

    WHAL_TEST_SUITE_START("flash_platform");
    WHAL_TEST(Test_Versal2Flash_Id);
    WHAL_TEST(Test_Versal2Flash_Program);
    WHAL_TEST(Test_Versal2Flash_DmaRead);
    WHAL_TEST(Test_Versal2Flash_StigRead);
    WHAL_TEST(Test_Versal2Flash_UnalignedRead);
    WHAL_TEST(Test_Versal2Flash_MisalignedRead);
    WHAL_TEST(Test_Versal2Flash_MapRead);
    WHAL_TEST(Test_Versal2Flash_DmaAfterMap);
    WHAL_TEST(Test_Versal2Flash_Erase);
    WHAL_TEST(Test_Versal2Flash_InitWrongId);
    WHAL_TEST(Test_Versal2Flash_InitBadCfg);
    WHAL_TEST(Test_Versal2Flash_WriteAcrossPages);
    WHAL_TEST(Test_Versal2Flash_EraseErrors);
    WHAL_TEST(Test_Versal2Flash_Bounds);
    WHAL_TEST(Test_Versal2Flash_LockUnlock);
    WHAL_TEST(Test_Versal2Flash_NoPartValues);
    WHAL_TEST(Test_Versal2Flash_OctalRecovery);
    WHAL_TEST(Test_Versal2Flash_ProgramError);
    WHAL_TEST(Test_Versal2Flash_EraseError);
    WHAL_TEST(Test_Versal2Flash_ErrorNoClearOp);
    WHAL_TEST(Test_Versal2Flash_Timeouts);
    WHAL_TEST(Test_Versal2Flash_FailEachCommand);
    WHAL_TEST(Test_Versal2Flash_Erase);
    WHAL_TEST_SUITE_END();
}
