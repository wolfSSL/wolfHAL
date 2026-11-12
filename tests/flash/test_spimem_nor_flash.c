/* test_spimem_nor_flash.c
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

/* Fake controller acting like a Micron MT35XU02G: busy drops all but 05h/70h,
 * WEL, page wrap, injectable FSR errors, 8 bytes per command (like STIG). */

#include <string.h>
#include <wolfHAL/flash/flash.h>
#include <wolfHAL/flash/spimem_nor_flash.h>
#include <wolfHAL/spimem/spimem.h>
#include <wolfHAL/timeout.h>
#include "../test.h"

#define NOR_PAGE_SZ   256
#define NOR_ERASE_SZ  0x20000          /* 128 KB, like the MT35XU02G */
#define NOR_BLOCKS    4
#define NOR_CAP       (NOR_ERASE_SZ * NOR_BLOCKS)
#define NOR_MAX_DATA  8                /* forces many program pieces */
#define NOR_BUSY_POLLS 3

#define OP_READ  0x13
#define OP_PROG  0x12
#define OP_ERASE 0xDC

/* Fake flash state */
static uint8_t g_nor[NOR_CAP];
static uint8_t g_id[3];
static int g_busy;        /* status reads left before ready */
static int g_wel;         /* write enable latch */
static int g_welStuck;    /* 06h never sets the latch */
static uint8_t g_fsrErr;  /* flag status error bits */
static uint8_t g_injectProgErr;
static uint8_t g_injectEraseErr;
static int g_dropped;     /* commands lost: busy, or no latch */
static int g_badOp;       /* malformed or unknown commands */
static int g_execCount;
static int g_progCount;
static int g_eraseCount;
static int g_clearCount;
static int g_fsrReads;
/* UNKNOWN reads FF until a 1-bit reset; OCTAL hears only octal DDR (1-bit
 * reads 0). A reset loads the nonvolatile address mode. */
#define NOR_MODE_1BIT    0
#define NOR_MODE_UNKNOWN 1
#define NOR_MODE_OCTAL   2
static int g_mode;
static int g_resetDead;   /* 99h does not bring the flash back */
static int g_addr4;       /* 4-byte address mode (flag status bit 0) */
static int g_nvAddr4;     /* address mode loaded by a reset */
static int g_rstEn;       /* 66h was the last command */
/* Command log: opcode and whether it was sent as octal DDR */
#define NOR_LOG 64
static uint8_t g_log[NOR_LOG];
static uint8_t g_logOctal[NOR_LOG];
static int g_nlog;
/* Last 81h write */
static int g_vcfgWrites;
static uint8_t g_vcfgAddrBytes;
static uint32_t g_vcfgAddr;
static uint8_t g_vcfgVal;

static void Nor_Reset(void)
{
    memset(g_nor, 0xFF, sizeof(g_nor));
    g_id[0] = 0x2C; /* Micron (Table 18) */
    g_id[1] = 0x5B;
    g_id[2] = 0x1C;
    g_busy = 0;
    g_wel = 0;
    g_welStuck = 0;
    g_fsrErr = 0;
    g_injectProgErr = 0;
    g_injectEraseErr = 0;
    g_dropped = 0;
    g_badOp = 0;
    g_execCount = 0;
    g_progCount = 0;
    g_eraseCount = 0;
    g_clearCount = 0;
    g_fsrReads = 0;
    g_mode = NOR_MODE_1BIT;
    g_resetDead = 0;
    g_addr4 = 1;
    g_nvAddr4 = 0;
    g_rstEn = 0;
    memset(g_log, 0, sizeof(g_log));
    memset(g_logOctal, 0, sizeof(g_logOctal));
    g_nlog = 0;
    g_vcfgWrites = 0;
    g_vcfgAddrBytes = 0;
    g_vcfgAddr = 0xFFFFFFFF;
    g_vcfgVal = 0;
}

/* RESET ENABLE then RESET MEMORY, in the mode the flash hears */
static void Nor_ResetCmd(uint8_t opcode)
{
    if (opcode == 0x99 && g_rstEn && !g_resetDead) {
        g_mode = NOR_MODE_1BIT;
        g_addr4 = g_nvAddr4;
        g_wel = 0;
        g_busy = NOR_BUSY_POLLS;
    }
    g_rstEn = 0;
    if (opcode == 0x66) {
        g_rstEn = 1;
    }
}

/* Position of the first logged opcode, -1 if not sent */
static int Nor_LogPos(uint8_t opcode, int from)
{
    int i;

    for (i = from; i < g_nlog && i < NOR_LOG; i++) {
        if (g_log[i] == opcode) {
            return i;
        }
    }
    return -1;
}

static whal_Error Nor_Program(const whal_SpiMemOp *op)
{
    const uint8_t *src = (const uint8_t *)op->data;
    size_t page;
    size_t i;

    g_progCount++;
    if (op->addrBytes != 4 || op->dummyCycles != 0 ||
        op->dir != WHAL_SPIMEM_DIR_OUT || op->dataSz == 0 ||
        op->dataSz > NOR_MAX_DATA || op->addr >= NOR_CAP) {
        g_badOp++;
        return WHAL_EINVAL;
    }
    /* A piece that crosses a page end would wrap on a real flash */
    if ((op->addr % NOR_PAGE_SZ) + op->dataSz > NOR_PAGE_SZ) {
        g_badOp++;
    }
    if (!g_wel) {
        g_dropped++;
        return WHAL_SUCCESS;
    }
    page = op->addr - (op->addr % NOR_PAGE_SZ);
    for (i = 0; i < op->dataSz; i++) {
        g_nor[page + ((op->addr + i) % NOR_PAGE_SZ)] &= src[i];
    }
    g_wel = 0;
    g_busy = NOR_BUSY_POLLS;
    g_fsrErr |= g_injectProgErr;
    g_injectProgErr = 0;
    return WHAL_SUCCESS;
}

static whal_Error Nor_Erase(const whal_SpiMemOp *op)
{
    g_eraseCount++;
    if (op->addrBytes != 4 || op->dir != WHAL_SPIMEM_DIR_NONE ||
        op->dataSz != 0 || op->addr >= NOR_CAP ||
        (op->addr % NOR_ERASE_SZ) != 0) {
        g_badOp++;
        return WHAL_EINVAL;
    }
    if (!g_wel) {
        g_dropped++;
        return WHAL_SUCCESS;
    }
    memset(&g_nor[op->addr], 0xFF, NOR_ERASE_SZ);
    g_wel = 0;
    g_busy = NOR_BUSY_POLLS;
    g_fsrErr |= g_injectEraseErr;
    g_injectEraseErr = 0;
    return WHAL_SUCCESS;
}

static whal_Error FakeSpiMemExec(whal_SpiMem *d, const whal_SpiMemOp *op)
{
    uint8_t *out = (uint8_t *)op->data;
    uint8_t val;
    size_t i;
    int octal = 0;

    (void)d;
    g_execCount++;

    /* The driver sends every command in 1-bit single rate (1-1-1), except
     * the octal DDR reset: an 8-line DTR opcode with no address or data. */
    if (op->opLines == 8 && op->dtr == 1 && op->addrBytes == 0 &&
        op->dir == WHAL_SPIMEM_DIR_NONE &&
        (op->opcode == 0x66 || op->opcode == 0x99)) {
        octal = 1;
    }
    else if (op->opLines != 1 || op->addrLines != 1 || op->dataLines != 1 ||
             op->dtr != 0) {
        g_badOp++;
        return WHAL_EINVAL;
    }
    if (g_nlog < NOR_LOG) {
        g_log[g_nlog] = op->opcode;
        g_logOctal[g_nlog] = (uint8_t)octal;
    }
    g_nlog++;

    /* A flash in octal DDR hears only octal commands; a flash in 1-bit
     * mode does not take an octal one. Reads that are not heard give 0. */
    if (octal != (g_mode == NOR_MODE_OCTAL)) {
        if (op->dir == WHAL_SPIMEM_DIR_IN) {
            for (i = 0; i < op->dataSz; i++) {
                out[i] = 0;
            }
        }
        return WHAL_SUCCESS;
    }
    if (octal) {
        Nor_ResetCmd(op->opcode);
        return WHAL_SUCCESS;
    }
    /* Unknown mode: only a 1-bit reset is taken; reads give FF */
    if (g_mode == NOR_MODE_UNKNOWN) {
        if (op->opcode == 0x66 || op->opcode == 0x99) {
            Nor_ResetCmd(op->opcode);
        }
        else if (op->dir == WHAL_SPIMEM_DIR_IN) {
            for (i = 0; i < op->dataSz; i++) {
                out[i] = 0xFF;
            }
        }
        return WHAL_SUCCESS;
    }
    if (op->opcode != 0x99) {
        g_rstEn = 0;
    }

    /* A busy flash only answers status reads */
    if (g_busy > 0 && op->opcode != 0x05 && op->opcode != 0x70) {
        g_dropped++;
        return WHAL_SUCCESS;
    }

    switch (op->opcode) {
    case 0x05:
        if (op->dataSz != 1 || op->dir != WHAL_SPIMEM_DIR_IN ||
            op->addrBytes != 0) {
            g_badOp++;
            return WHAL_EINVAL;
        }
        val = 0;
        if (g_busy > 0) {
            val |= 0x01;
            g_busy--;
        }
        if (g_wel) {
            val |= 0x02;
        }
        out[0] = val;
        return WHAL_SUCCESS;
    case 0x70:
        if (op->dataSz != 1 || op->dir != WHAL_SPIMEM_DIR_IN) {
            g_badOp++;
            return WHAL_EINVAL;
        }
        g_fsrReads++;
        /* bit 7 = ready, bit 0 = 4-byte addressing */
        val = (uint8_t)(0x80 | g_fsrErr);
        if (g_addr4) {
            val |= 0x01;
        }
        if (g_busy > 0) {
            val = 0x01;
            g_busy--;
        }
        out[0] = val;
        return WHAL_SUCCESS;
    case 0x06:
        if (op->dataSz != 0 || op->addrBytes != 0) {
            g_badOp++;
            return WHAL_EINVAL;
        }
        if (!g_welStuck) {
            g_wel = 1;
        }
        return WHAL_SUCCESS;
    case 0x50:
        if (op->dataSz != 0 || op->addrBytes != 0) {
            g_badOp++;
            return WHAL_EINVAL;
        }
        g_clearCount++;
        g_fsrErr = 0;
        return WHAL_SUCCESS;
    case 0x9F:
        if (op->dataSz != 3 || op->dir != WHAL_SPIMEM_DIR_IN ||
            op->addrBytes != 0) {
            g_badOp++;
            return WHAL_EINVAL;
        }
        memcpy(out, g_id, 3);
        return WHAL_SUCCESS;
    case 0x66:
    case 0x99:
        if (op->dataSz != 0 || op->addrBytes != 0) {
            g_badOp++;
            return WHAL_EINVAL;
        }
        Nor_ResetCmd(op->opcode);
        return WHAL_SUCCESS;
    case 0x81:
        /* Volatile configuration write: one byte, 3 or 4 address bytes as
         * the address mode says */
        if (op->dir != WHAL_SPIMEM_DIR_OUT || op->dataSz != 1) {
            g_badOp++;
            return WHAL_EINVAL;
        }
        if (!g_wel) {
            g_dropped++;
            return WHAL_SUCCESS;
        }
        g_vcfgWrites++;
        g_vcfgAddrBytes = op->addrBytes;
        g_vcfgAddr = op->addr;
        g_vcfgVal = ((const uint8_t *)op->data)[0];
        g_wel = 0;
        return WHAL_SUCCESS;
    case OP_READ:
        if (op->addrBytes != 4 || op->dummyCycles != 0 ||
            op->dir != WHAL_SPIMEM_DIR_IN || op->addr > NOR_CAP ||
            op->dataSz > NOR_CAP - op->addr) {
            g_badOp++;
            return WHAL_EINVAL;
        }
        memcpy(out, &g_nor[op->addr], op->dataSz);
        return WHAL_SUCCESS;
    case OP_PROG:
        return Nor_Program(op);
    case OP_ERASE:
        return Nor_Erase(op);
    default:
        g_badOp++;
        return WHAL_EINVAL;
    }
}

static whal_Error FakeSpiMemMaxDataSz(whal_SpiMem *d, const whal_SpiMemOp *op,
                                      size_t *maxSz)
{
    (void)d;
    (void)op;
    *maxSz = NOR_MAX_DATA;
    return WHAL_SUCCESS;
}

static const whal_SpiMemDriver g_fakeSpiMemDriver = {
    .Exec = FakeSpiMemExec,
    .MaxDataSz = FakeSpiMemMaxDataSz,
};

static whal_SpiMem g_spiMem = {
    .driver = &g_fakeSpiMemDriver,
};

/* Fake tick: moves on every read, so a stuck poll times out */
#ifdef WHAL_CFG_64BIT_TICK
static uint64_t g_tick;

static uint64_t NorTick(void)
{
    return g_tick++;
}
#else
static uint32_t g_tick;

static uint32_t NorTick(void)
{
    return g_tick++;
}
#endif

static whal_Timeout g_timeout = {
    .timeoutTicks = 1000,
    .GetTick = NorTick,
};

static whal_SpiMemNor_Flash_Cfg g_cfg;

static whal_Flash g_flash = {
    .driver = &whal_SpiMemNor_Flash_Driver,
    .cfg = &g_cfg,
};

static uint8_t g_buf[1024];
static uint8_t g_ref[1024];

/* Fresh flash and config with the MT35XU02G settings, then Init */
static whal_Error Setup(void)
{
    Nor_Reset();
    g_tick = 0;
    g_timeout.timeoutTicks = 1000;
    g_cfg.spiMemDev = &g_spiMem;
    g_cfg.capacity = NOR_CAP;
    g_cfg.pageSz = NOR_PAGE_SZ;
    g_cfg.eraseSz = NOR_ERASE_SZ;
    g_cfg.addrBytes = 4;
    g_cfg.readOp = OP_READ;
    g_cfg.readDummy = 0;
    g_cfg.progOp = OP_PROG;
    g_cfg.eraseOp = OP_ERASE;
    g_cfg.flags = WHAL_SPIMEM_NOR_FLAG_FSR;
    g_cfg.id = 0;
    g_cfg.timeout = &g_timeout;
    return whal_Flash_Init(&g_flash);
}

static void Fill(uint8_t *p, size_t n, uint8_t seed)
{
    size_t i;

    for (i = 0; i < n; i++) {
        p[i] = (uint8_t)(seed + i * 7);
    }
}

static int AllFF(size_t addr, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        if (g_nor[addr + i] != 0xFF) {
            return 0;
        }
    }
    return 1;
}

static void Test_SpiMemNor_InitOk(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    /* 50h clears old errors */
    WHAL_ASSERT_EQ(g_clearCount, 1);
    WHAL_ASSERT_EQ(g_badOp, 0);
}

static void Test_SpiMemNor_InitIdZero(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    memset(g_id, 0x00, sizeof(g_id));
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_EHARDWARE);
}

static void Test_SpiMemNor_InitIdFF(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    memset(g_id, 0xFF, sizeof(g_id));
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_EHARDWARE);
}

#define NOR_RESET_FLAGS (WHAL_SPIMEM_NOR_FLAG_FSR | WHAL_SPIMEM_NOR_FLAG_RESET | \
                         WHAL_SPIMEM_NOR_FLAG_MICRON_VCFG_1BIT)

/* A flash that answers in 1-bit mode is not reset */
static void Test_SpiMemNor_InitNoResetNeeded(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Nor_Reset();
    g_cfg.flags = NOR_RESET_FLAGS | WHAL_SPIMEM_NOR_FLAG_RESET_OCTAL_DTR;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Nor_LogPos(0x66, 0), -1);
    WHAL_ASSERT_EQ(g_vcfgWrites, 0);
    WHAL_ASSERT_EQ(g_badOp, 0);
}

/* A flash in an unknown mode: 1-bit reset, wait, then the volatile 1-bit
 * setting with 3 address bytes (the reset loads 3-byte mode) */
static void Test_SpiMemNor_InitResetUnknownMode(void)
{
    int p66, p99, p06, p81;

    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Nor_Reset();
    g_mode = NOR_MODE_UNKNOWN;
    g_cfg.flags = NOR_RESET_FLAGS;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_mode, NOR_MODE_1BIT);
    p66 = Nor_LogPos(0x66, 0);
    p99 = Nor_LogPos(0x99, 0);
    p06 = Nor_LogPos(0x06, 0);
    p81 = Nor_LogPos(0x81, 0);
    WHAL_ASSERT_EQ(p66 > 0 && p99 > p66 && p06 > p99 && p81 > p06, 1);
    WHAL_ASSERT_EQ(g_vcfgWrites, 1);
    WHAL_ASSERT_EQ(g_vcfgAddrBytes, 3);
    WHAL_ASSERT_EQ(g_vcfgAddr, 0x00);
    WHAL_ASSERT_EQ(g_vcfgVal, 0xFF);
    /* Never the nonvolatile configuration write */
    WHAL_ASSERT_EQ(Nor_LogPos(0xB1, 0), -1);
    WHAL_ASSERT_EQ(g_badOp, 0);
}

/* The volatile write follows a 4-byte address mode after the reset */
static void Test_SpiMemNor_InitVcfg4ByteMode(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Nor_Reset();
    g_mode = NOR_MODE_UNKNOWN;
    g_nvAddr4 = 1;
    g_cfg.flags = NOR_RESET_FLAGS;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_vcfgAddrBytes, 4);
}

/* A flash left in octal DDR: the reset goes out in octal DDR first, then
 * in 1-bit */
static void Test_SpiMemNor_InitResetOctal(void)
{
    int i;

    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Nor_Reset();
    g_mode = NOR_MODE_OCTAL;
    g_cfg.flags = NOR_RESET_FLAGS | WHAL_SPIMEM_NOR_FLAG_RESET_OCTAL_DTR;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_mode, NOR_MODE_1BIT);
    /* 1-bit ID (not heard), octal 66h and 99h, then 1-bit 66h and 99h */
    WHAL_ASSERT_EQ(g_log[0], 0x9F);
    WHAL_ASSERT_EQ(g_log[1], 0x66);
    WHAL_ASSERT_EQ(g_log[2], 0x99);
    WHAL_ASSERT_EQ(g_logOctal[1] && g_logOctal[2], 1);
    WHAL_ASSERT_EQ(g_log[3], 0x66);
    WHAL_ASSERT_EQ(g_log[4], 0x99);
    for (i = 3; i < g_nlog && i < NOR_LOG; i++) {
        WHAL_ASSERT_EQ(g_logOctal[i], 0);
    }
    WHAL_ASSERT_EQ(g_badOp, 0);
}

/* Without the octal flag a flash in octal DDR is not reached */
static void Test_SpiMemNor_InitOctalNeedsFlag(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Nor_Reset();
    g_mode = NOR_MODE_OCTAL;
    g_cfg.flags = NOR_RESET_FLAGS;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(g_mode, NOR_MODE_OCTAL);
}

/* Without FLAG_RESET nothing is sent to recover the flash */
static void Test_SpiMemNor_InitNoResetFlag(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Nor_Reset();
    g_mode = NOR_MODE_UNKNOWN;
    g_cfg.flags = WHAL_SPIMEM_NOR_FLAG_FSR;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(Nor_LogPos(0x66, 0), -1);
    WHAL_ASSERT_EQ(Nor_LogPos(0x81, 0), -1);
}

/* With cfg->id, only that part is accepted */
static void Test_SpiMemNor_InitExpectedId(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Nor_Reset();
    g_cfg.id = 0x1C5B2C;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_SUCCESS);

    /* Micron 3 V part (2C 5A 1C): refused, also after a reset */
    Nor_Reset();
    g_id[1] = 0x5A;
    g_cfg.flags = NOR_RESET_FLAGS;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(Nor_LogPos(0x99, 0) >= 0, 1);
}

static void Test_SpiMemNor_ReadId(void)
{
    uint8_t id[3] = {0, 0, 0};

    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_ReadId(&g_flash, id), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(id[0] == 0x2C && id[1] == 0x5B && id[2] == 0x1C, 1);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_ReadId(&g_flash, NULL), WHAL_EINVAL);
}

static void Test_SpiMemNor_InitBadCfg(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    g_cfg.pageSz = 100;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_EINVAL);
    g_cfg.pageSz = NOR_PAGE_SZ;
    g_cfg.eraseSz = 0;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_EINVAL);
    g_cfg.eraseSz = NOR_ERASE_SZ;
    g_cfg.addrBytes = 2;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_EINVAL);
    g_cfg.addrBytes = 4;
    g_cfg.capacity = 0;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_EINVAL);
}

static void Test_SpiMemNor_NullArgs(void)
{
    whal_Flash noCfg = {
        .driver = &whal_SpiMemNor_Flash_Driver,
    };

    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Deinit(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Lock(NULL, 0, 0), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Unlock(NULL, 0, 0), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Read(NULL, 0, g_buf, 4), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Write(NULL, 0, g_buf, 4), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_SpiMemNor_Flash_Erase(NULL, 0, NOR_ERASE_SZ),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Init(&noCfg), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Read(&noCfg, 0, g_buf, 4), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_flash, 0, NULL, 4), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, 0, NULL, 4), WHAL_EINVAL);
}

static void Test_SpiMemNor_ZeroLength(void)
{
    int before;

    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    before = g_execCount;
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_flash, 0, g_buf, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, 0, g_buf, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, 0, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_execCount, before);
}

static void Test_SpiMemNor_LockUnlockNoCommands(void)
{
    int before;

    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    before = g_execCount;
    WHAL_ASSERT_EQ(whal_Flash_Lock(&g_flash, 0, NOR_CAP), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Unlock(&g_flash, 0, NOR_CAP), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Deinit(&g_flash), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_execCount, before);
}

static void Test_SpiMemNor_Read(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Fill(&g_nor[3], 1000, 0x11);
    memset(g_buf, 0, sizeof(g_buf));
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_flash, 3, g_buf, 1000), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(g_buf, &g_nor[3], 1000);
    WHAL_ASSERT_EQ(g_badOp, 0);
}

static void Test_SpiMemNor_ReadOutOfRange(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_flash, NOR_CAP - 4, g_buf, 8),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_flash, NOR_CAP + 1, g_buf, 1),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_flash, NOR_CAP - 4, g_buf, 4),
                   WHAL_SUCCESS);
}

/* One full page: 32 pieces of 8 bytes */
static void Test_SpiMemNor_WritePage(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Fill(g_ref, NOR_PAGE_SZ, 0x40);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, NOR_PAGE_SZ, g_ref,
                                    NOR_PAGE_SZ), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(&g_nor[NOR_PAGE_SZ], g_ref, NOR_PAGE_SZ);
    WHAL_ASSERT_EQ(g_progCount, NOR_PAGE_SZ / NOR_MAX_DATA);
    WHAL_ASSERT_EQ(g_dropped, 0);
    WHAL_ASSERT_EQ(g_badOp, 0);
    WHAL_ASSERT_NEQ(AllFF(0, NOR_PAGE_SZ), 0);
    WHAL_ASSERT_NEQ(AllFF(2 * NOR_PAGE_SZ, NOR_PAGE_SZ), 0);
}

/* 600 bytes starting 100 bytes before a page end, over three pages */
static void Test_SpiMemNor_WriteOddStart(void)
{
    const size_t start = NOR_PAGE_SZ - 100;
    const size_t len = 600;

    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Fill(g_ref, len, 0x5A);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, start, g_ref, len),
                   WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(&g_nor[start], g_ref, len);
    WHAL_ASSERT_NEQ(AllFF(0, start), 0);
    WHAL_ASSERT_NEQ(AllFF(start + len, 1024 - (start + len)), 0);
    /* 100 bytes = 13 pieces (12 x 8 + 4), 256 bytes = 32 pieces,
     * 244 bytes = 31 pieces (30 x 8 + 4) */
    WHAL_ASSERT_EQ(g_progCount, 13 + 32 + 31);
    WHAL_ASSERT_EQ(g_dropped, 0);
    WHAL_ASSERT_EQ(g_badOp, 0);
    /* One flag status check per piece */
    WHAL_ASSERT_EQ(g_fsrReads, 13 + 32 + 31);
}

/* A short write inside one page with an odd start */
static void Test_SpiMemNor_WriteShort(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Fill(g_ref, 5, 0x01);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, 0x20003, g_ref, 5),
                   WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(&g_nor[0x20003], g_ref, 5);
    WHAL_ASSERT_EQ(g_nor[0x20002], 0xFF);
    WHAL_ASSERT_EQ(g_nor[0x20008], 0xFF);
    WHAL_ASSERT_EQ(g_progCount, 1);
    WHAL_ASSERT_EQ(g_dropped, 0);
}

static void Test_SpiMemNor_WriteOutOfRange(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, NOR_CAP - 4, g_ref, 8),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(g_progCount, 0);
}

static void Test_SpiMemNor_WriteThenRead(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Fill(g_ref, sizeof(g_ref), 0x33);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, NOR_ERASE_SZ + 77, g_ref,
                                    sizeof(g_ref)), WHAL_SUCCESS);
    memset(g_buf, 0, sizeof(g_buf));
    WHAL_ASSERT_EQ(whal_Flash_Read(&g_flash, NOR_ERASE_SZ + 77, g_buf,
                                   sizeof(g_buf)), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(g_buf, g_ref, sizeof(g_ref));
    WHAL_ASSERT_EQ(g_dropped, 0);
}

static void Test_SpiMemNor_EraseOne(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    memset(g_nor, 0x00, sizeof(g_nor));
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, NOR_ERASE_SZ, NOR_ERASE_SZ),
                   WHAL_SUCCESS);
    WHAL_ASSERT_NEQ(AllFF(NOR_ERASE_SZ, NOR_ERASE_SZ), 0);
    WHAL_ASSERT_EQ(g_nor[NOR_ERASE_SZ - 1], 0x00);
    WHAL_ASSERT_EQ(g_nor[2 * NOR_ERASE_SZ], 0x00);
    WHAL_ASSERT_EQ(g_eraseCount, 1);
    WHAL_ASSERT_EQ(g_dropped, 0);
    WHAL_ASSERT_EQ(g_badOp, 0);
}

static void Test_SpiMemNor_EraseSeveral(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    memset(g_nor, 0x00, sizeof(g_nor));
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, NOR_ERASE_SZ,
                                    3 * NOR_ERASE_SZ), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_nor[NOR_ERASE_SZ - 1], 0x00);
    WHAL_ASSERT_NEQ(AllFF(NOR_ERASE_SZ, 3 * NOR_ERASE_SZ), 0);
    WHAL_ASSERT_EQ(g_eraseCount, 3);
    WHAL_ASSERT_EQ(g_dropped, 0);
    WHAL_ASSERT_EQ(g_badOp, 0);
}

static void Test_SpiMemNor_EraseMisaligned(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, 0x1000, NOR_ERASE_SZ),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, 0, NOR_ERASE_SZ + 1),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, 0, 4096), WHAL_EINVAL);
    WHAL_ASSERT_EQ(g_eraseCount, 0);
}

static void Test_SpiMemNor_EraseOutOfRange(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, NOR_CAP, NOR_ERASE_SZ),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, 3 * NOR_ERASE_SZ,
                                    2 * NOR_ERASE_SZ), WHAL_EINVAL);
    WHAL_ASSERT_EQ(g_eraseCount, 0);
}

/* A program error is reported, cleared with 50h, and the next write works */
static void Test_SpiMemNor_FsrProgramError(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    g_clearCount = 0;
    g_injectProgErr = 0x10;
    Fill(g_ref, 16, 0x77);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, 0, g_ref, 16), WHAL_EHARDWARE);
    /* It stops after the first piece */
    WHAL_ASSERT_EQ(g_progCount, 1);
    WHAL_ASSERT_EQ(g_clearCount, 1);
    WHAL_ASSERT_EQ(g_fsrErr, 0);
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, 0x100, g_ref, 16),
                   WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(&g_nor[0x100], g_ref, 16);
    WHAL_ASSERT_EQ(g_dropped, 0);
}

static void Test_SpiMemNor_FsrProtectError(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    g_clearCount = 0;
    g_injectProgErr = 0x02;
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, 0, g_ref, 4), WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(g_clearCount, 1);
}

static void Test_SpiMemNor_FsrEraseError(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    g_clearCount = 0;
    g_injectEraseErr = 0x20;
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, 0, 2 * NOR_ERASE_SZ),
                   WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(g_eraseCount, 1);
    WHAL_ASSERT_EQ(g_clearCount, 1);
    WHAL_ASSERT_EQ(g_fsrErr, 0);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, 0, NOR_ERASE_SZ),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_dropped, 0);
}

/* Without FLAG_FSR the driver never reads 70h or sends 50h */
static void Test_SpiMemNor_NoFsr(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    g_cfg.flags = 0;
    g_clearCount = 0;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_SUCCESS);
    g_injectProgErr = 0x10;
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, 0, g_ref, 4), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_fsrReads, 0);
    WHAL_ASSERT_EQ(g_clearCount, 0);
    WHAL_ASSERT_EQ(g_dropped, 0);
}

#ifndef WHAL_CFG_NO_TIMEOUT
/* The latch never sets: the write times out and sends no program */
static void Test_SpiMemNor_WelTimeout(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    g_timeout.timeoutTicks = 20;
    g_welStuck = 1;
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, 0, g_ref, 4), WHAL_ETIMEOUT);
    WHAL_ASSERT_EQ(g_progCount, 0);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&g_flash, 0, NOR_ERASE_SZ),
                   WHAL_ETIMEOUT);
    WHAL_ASSERT_EQ(g_eraseCount, 0);
}

/* The flash stays busy: the wait times out */
/* A flash the reset does not bring back: it never gets ready (reads FF),
 * so Init times out. Without timeouts the status wait would not end. */
static void Test_SpiMemNor_InitResetDead(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    Nor_Reset();
    g_mode = NOR_MODE_UNKNOWN;
    g_resetDead = 1;
    g_cfg.flags = NOR_RESET_FLAGS;
    g_timeout.timeoutTicks = 20;
    WHAL_ASSERT_EQ(whal_Flash_Init(&g_flash), WHAL_ETIMEOUT);
    WHAL_ASSERT_EQ(Nor_LogPos(0x99, 0) >= 0, 1);
}

static void Test_SpiMemNor_BusyTimeout(void)
{
    WHAL_ASSERT_EQ(Setup(), WHAL_SUCCESS);
    g_timeout.timeoutTicks = 20;
    g_busy = 1000000;
    WHAL_ASSERT_EQ(whal_Flash_Write(&g_flash, 0, g_ref, 4), WHAL_ETIMEOUT);
    WHAL_ASSERT_EQ(g_progCount, 0);
}
#endif /* !WHAL_CFG_NO_TIMEOUT */

void whal_Test_SpiMemNor_Flash(void)
{
    WHAL_TEST_SUITE_START("spimem_nor_flash");
    WHAL_TEST(Test_SpiMemNor_InitOk);
    WHAL_TEST(Test_SpiMemNor_InitIdZero);
    WHAL_TEST(Test_SpiMemNor_InitIdFF);
    WHAL_TEST(Test_SpiMemNor_InitNoResetNeeded);
    WHAL_TEST(Test_SpiMemNor_InitResetUnknownMode);
    WHAL_TEST(Test_SpiMemNor_InitVcfg4ByteMode);
    WHAL_TEST(Test_SpiMemNor_InitResetOctal);
    WHAL_TEST(Test_SpiMemNor_InitOctalNeedsFlag);
    WHAL_TEST(Test_SpiMemNor_InitNoResetFlag);
    WHAL_TEST(Test_SpiMemNor_InitExpectedId);
    WHAL_TEST(Test_SpiMemNor_ReadId);
    WHAL_TEST(Test_SpiMemNor_InitBadCfg);
    WHAL_TEST(Test_SpiMemNor_NullArgs);
    WHAL_TEST(Test_SpiMemNor_ZeroLength);
    WHAL_TEST(Test_SpiMemNor_LockUnlockNoCommands);
    WHAL_TEST(Test_SpiMemNor_Read);
    WHAL_TEST(Test_SpiMemNor_ReadOutOfRange);
    WHAL_TEST(Test_SpiMemNor_WritePage);
    WHAL_TEST(Test_SpiMemNor_WriteOddStart);
    WHAL_TEST(Test_SpiMemNor_WriteShort);
    WHAL_TEST(Test_SpiMemNor_WriteOutOfRange);
    WHAL_TEST(Test_SpiMemNor_WriteThenRead);
    WHAL_TEST(Test_SpiMemNor_EraseOne);
    WHAL_TEST(Test_SpiMemNor_EraseSeveral);
    WHAL_TEST(Test_SpiMemNor_EraseMisaligned);
    WHAL_TEST(Test_SpiMemNor_EraseOutOfRange);
    WHAL_TEST(Test_SpiMemNor_FsrProgramError);
    WHAL_TEST(Test_SpiMemNor_FsrProtectError);
    WHAL_TEST(Test_SpiMemNor_FsrEraseError);
    WHAL_TEST(Test_SpiMemNor_NoFsr);
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_TEST(Test_SpiMemNor_WelTimeout);
    WHAL_TEST(Test_SpiMemNor_BusyTimeout);
    WHAL_TEST(Test_SpiMemNor_InitResetDead);
#endif
    WHAL_TEST_SUITE_END();
}
