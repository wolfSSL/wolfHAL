/* test_cadence_ospi_spimem.c
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

/* Emulated registers. The tick source runs the emulator (after each
 * FLASH_CMD_CTRL write); without it, multi-command tests are skipped. */

#include <stdint.h>
#include <string.h>
#include <wolfHAL/spimem/spimem.h>
#include <wolfHAL/spimem/cadence_ospi_spimem.h>
#include <wolfHAL/platform/amd/versal2.h>
#include "../test.h"

/* Register offsets and bits (AM028), copied here since the driver keeps
 * them private. */
#define T_CONFIG          0x00
#define T_DEV_INSTR_RD    0x04
#define T_DEV_INSTR_WR    0x08
#define T_RD_CAPTURE      0x10
#define T_DEV_SIZE        0x14
#define T_SRAM_PARTITION  0x18
#define T_IND_AHB_TRIG    0x1C
#define T_DMA_PERIPH      0x20
#define T_IND_RD_CTRL     0x60
#define T_IND_RD_WMARK    0x64
#define T_IND_RD_START    0x68
#define T_IND_RD_NUM      0x6C
#define T_IND_TRIG_RANGE  0x80
#define T_CMD_CTRL        0x90
#define T_CMD_ADDR        0x94
#define T_RD_LO           0xA0
#define T_RD_HI           0xA4
#define T_WR_LO           0xA8
#define T_WR_HI           0xAC

#define T_CFG_ENB_SPI     (1UL << 0)
#define T_CFG_PHY         (1UL << 3)
#define T_CFG_CS_Pos      10
#define T_CFG_CS_Msk      (0xFUL << 10)
#define T_CFG_DMA_IF      (1UL << 15)
#define T_CFG_BAUD_Pos    19
#define T_CFG_BAUD_Msk    (0xFUL << 19)
#define T_CFG_DTR         (1UL << 24)
#define T_CFG_PIPELINE_PHY (1UL << 25)
#define T_CFG_DUAL_OPCODE (1UL << 30)
#define T_CFG_IDLE        (1UL << 31)

#define T_RD_INSTR_Pos    8
#define T_RD_ADDR_Pos     12
#define T_RD_DATA_Pos     16
#define T_WR_ADDR_Pos     12
#define T_WR_DATA_Pos     16

#define T_EXEC            (1UL << 0)
#define T_EXEC_STATUS     (1UL << 1)
#define T_DUMMY_Pos       7
#define T_WR_BYTES_Pos    12
#define T_ENB_WRITE       (1UL << 15)
#define T_ADDR_BYTES_Pos  16
#define T_ENB_ADDR        (1UL << 19)
#define T_RD_BYTES_Pos    20
#define T_ENB_READ        (1UL << 23)
#define T_OPCODE_Pos      24

#define T_IND_RD_START_BIT  (1UL << 0)
#define T_IND_RD_CANCEL_BIT (1UL << 1)

/* AM028 config_reg reset value: idle, /32 clock, DAC on, controller on. */
#define T_CONFIG_RESET    0x80780081UL

/* Emulated register block. */
static uint32_t g_regs[0x100 / 4] __attribute__((aligned(8)));
#define REG(off) g_regs[(off) / 4]

static uint32_t Field(uint32_t v, int pos, uint32_t mask)
{
    return (v >> pos) & mask;
}

/* Flash content seen by reads: byte at address a is Pat(a). */
static uint8_t Pat(uint32_t a)
{
    return (uint8_t)(a * 3 + 1);
}

/* Load the STIG read data registers with 8 bytes of Pat at a. */
static void LoadReadData(uint32_t a)
{
    uint32_t lo = 0;
    uint32_t hi = 0;
    int i;

    for (i = 3; i >= 0; i--) {
        lo = (lo << 8) | Pat(a + (uint32_t)i);
        hi = (hi << 8) | Pat(a + 4 + (uint32_t)i);
    }
    REG(T_RD_LO) = lo;
    REG(T_RD_HI) = hi;
}

/* Last STIG command control value (without EXEC/status), the same with
 * or without the emulator. */
static uint32_t LastCtrl(void)
{
    return REG(T_CMD_CTRL) & ~(uint32_t)(T_EXEC | T_EXEC_STATUS);
}

/* --- Emulator, run from the tick source --- */

#define EMU_CMDS 64
static uint32_t g_ctrl[EMU_CMDS];
static uint32_t g_addr[EMU_CMDS];
static uint64_t g_wdata[EMU_CMDS];
static int g_ncmd;
static int g_statusPolls;   /* steps until the done bit clears */
static int g_idlePolls;     /* more steps until CONFIG idle is set */
static int g_statusLag;
static int g_idleLag;
static int g_stuck;         /* never finish the command */
static int g_wrDataLost;    /* commands sent while not idle */
static int g_steps;

static void Emu_Step(void)
{
    uint32_t ctrl = REG(T_CMD_CTRL);

    g_steps++;
    if (ctrl & T_EXEC) {
        uint64_t w = (uint64_t)REG(T_WR_LO) | ((uint64_t)REG(T_WR_HI) << 32);

        /* A command set up while the controller is not idle loses its
         * write data (seen on the VEK385). */
        if ((REG(T_CONFIG) & T_CFG_IDLE) == 0) {
            g_wrDataLost++;
            w = 0;
        }
        if (g_ncmd < EMU_CMDS) {
            g_ctrl[g_ncmd] = ctrl & ~(uint32_t)T_EXEC;
            g_addr[g_ncmd] = REG(T_CMD_ADDR);
            g_wdata[g_ncmd] = w;
        }
        g_ncmd++;
        LoadReadData(REG(T_CMD_ADDR));
        REG(T_CMD_CTRL) = (ctrl & ~(uint32_t)T_EXEC) | T_EXEC_STATUS;
        REG(T_CONFIG) &= ~(uint32_t)T_CFG_IDLE;
        g_statusLag = g_statusPolls;
        g_idleLag = g_idlePolls;
    }
    if (g_stuck) {
        return;
    }
    if (REG(T_CMD_CTRL) & T_EXEC_STATUS) {
        if (g_statusLag > 0) {
            g_statusLag--;
            return;
        }
        REG(T_CMD_CTRL) &= ~(uint32_t)T_EXEC_STATUS;
    }
    if ((REG(T_CONFIG) & T_CFG_IDLE) == 0) {
        if (g_idleLag > 0) {
            g_idleLag--;
            return;
        }
        REG(T_CONFIG) |= T_CFG_IDLE;
    }
}

#ifdef WHAL_CFG_64BIT_TICK
static uint64_t g_tick;

static uint64_t EmuTick(void)
{
    Emu_Step();
    return ++g_tick;
}
#else
static uint32_t g_tick;

static uint32_t EmuTick(void)
{
    Emu_Step();
    return ++g_tick;
}
#endif

static whal_Timeout g_timeout = {
    .timeoutTicks = 1000,
    .GetTick = EmuTick,
};

/* Skip a test that needs the emulator to run a sequence of commands. */
#ifdef WHAL_CFG_NO_TIMEOUT
#define NEED_EMU() WHAL_SKIP()
#else
#define NEED_EMU() do { } while (0)
#endif

/* --- Fake DMA hook --- */

static int g_armCalls;
static int g_waitCalls;
static int g_seq;
static int g_armSeq;
static int g_waitSeq;
static void *g_armBuf;
static size_t g_armLen;
static void *g_waitBuf;
static size_t g_waitLen;
static uint32_t g_armIndCtrl;    /* IND_RD_XFER_CTRL when Arm runs */
static uint32_t g_waitIndCtrl;   /* IND_RD_XFER_CTRL when Wait runs */
static uint32_t g_armRdConfig;   /* DEV_INSTR_RD_CONFIG when Arm runs */
static whal_Error g_armRet;
static whal_Error g_waitRet;
static int g_dmaCtx;
/* Address bytes the indirect read sends: DEV_SIZE_CONFIG[3:0] + 1 when
 * Wait runs (AM028 dev_size_config_reg num_addr_bytes_fld). */
static uint32_t g_waitAddrBytes;

static whal_Error FakeArm(void *ctx, void *buf, size_t len)
{
    if (ctx != &g_dmaCtx) {
        return WHAL_EINVAL;
    }
    g_armCalls++;
    g_armSeq = ++g_seq;
    g_armBuf = buf;
    g_armLen = len;
    g_armIndCtrl = REG(T_IND_RD_CTRL);
    g_armRdConfig = REG(T_DEV_INSTR_RD);
    return g_armRet;
}

static whal_Error FakeWait(void *ctx, void *buf, size_t len)
{
    size_t i;
    uint32_t a = REG(T_IND_RD_START);

    if (ctx != &g_dmaCtx) {
        return WHAL_EINVAL;
    }
    g_waitCalls++;
    g_waitSeq = ++g_seq;
    g_waitBuf = buf;
    g_waitLen = len;
    g_waitIndCtrl = REG(T_IND_RD_CTRL);
    g_waitAddrBytes = (REG(T_DEV_SIZE) & 0xF) + 1;
    if (g_waitRet != WHAL_SUCCESS) {
        return g_waitRet;
    }
    for (i = 0; i < len; i++) {
        ((uint8_t *)buf)[i] = Pat(a + (uint32_t)i);
    }
    return WHAL_SUCCESS;
}

static const whal_CadenceOspi_SpiMem_Dma g_dma = {
    .Arm = FakeArm,
    .Wait = FakeWait,
    .ctx = &g_dmaCtx,
    .align = 64,
};

static whal_CadenceOspi_SpiMem_Cfg g_cfg;

static uint8_t g_buf[512] __attribute__((aligned(64)));

#define DEV_INIT {                                  \
    .base = (size_t)(uintptr_t)g_regs,              \
    .driver = &whal_CadenceOspi_SpiMem_Driver,      \
    .cfg = &g_cfg,                                  \
}

static void Setup(void)
{
    memset(g_regs, 0, sizeof(g_regs));
    REG(T_CONFIG) = T_CONFIG_RESET;
    REG(T_DEV_INSTR_RD) = 0x3;

    memset(g_ctrl, 0, sizeof(g_ctrl));
    memset(g_addr, 0, sizeof(g_addr));
    memset(g_wdata, 0, sizeof(g_wdata));
    g_ncmd = 0;
    g_statusPolls = 0;
    g_idlePolls = 0;
    g_statusLag = 0;
    g_idleLag = 0;
    g_stuck = 0;
    g_wrDataLost = 0;
    g_steps = 0;
    g_tick = 0;

    g_armCalls = 0;
    g_waitCalls = 0;
    g_seq = 0;
    g_armSeq = 0;
    g_waitSeq = 0;
    g_armBuf = NULL;
    g_armLen = 0;
    g_waitBuf = NULL;
    g_waitLen = 0;
    g_armIndCtrl = 0xFFFFFFFF;
    g_waitIndCtrl = 0xFFFFFFFF;
    g_armRdConfig = 0;
    g_waitAddrBytes = 0;
    g_armRet = WHAL_SUCCESS;
    g_waitRet = WHAL_SUCCESS;

    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.devSize = WHAL_VERSAL2_OSPI_DEV_SIZE_VEK385;
    g_cfg.indTrigBase = WHAL_VERSAL2_OSPI_IND_TRIG_BASE;
    g_cfg.indTrigRange = WHAL_VERSAL2_OSPI_IND_TRIG_RANGE;
    g_cfg.sramPartition = WHAL_VERSAL2_OSPI_SRAM_PARTITION;
    g_cfg.dmaPeriphCfg = WHAL_VERSAL2_OSPI_DMA_PERIPH_CFG;
    g_cfg.timeout = &g_timeout;

    memset(g_buf, 0, sizeof(g_buf));
}

static whal_Error Run(whal_SpiMem *dev, uint8_t opcode, uint8_t addrBytes,
                      uint8_t dummy, uint8_t dir, uint32_t addr, void *data,
                      size_t len)
{
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;

    op.opcode = opcode;
    op.addrBytes = addrBytes;
    op.dummyCycles = dummy;
    op.dir = dir;
    op.addr = addr;
    op.data = data;
    op.dataSz = len;
    return whal_SpiMem_Exec(dev, &op);
}

/* --- STIG register use --- */

static void Test_CadenceOspi_StigReadRegisters(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint8_t id[3];
    uint32_t c;

    Setup();
    LoadReadData(0);
    WHAL_ASSERT_EQ(Run(&dev, 0x9F, 0, 0, WHAL_SPIMEM_DIR_IN, 0, id, 3),
                   WHAL_SUCCESS);
    c = LastCtrl();
    WHAL_ASSERT_EQ(Field(c, T_OPCODE_Pos, 0xFF), 0x9F);
    WHAL_ASSERT_EQ((c & T_ENB_READ) != 0, 1);
    WHAL_ASSERT_EQ(Field(c, T_RD_BYTES_Pos, 7), 2);
    WHAL_ASSERT_EQ(c & T_ENB_ADDR, 0);
    WHAL_ASSERT_EQ(c & T_ENB_WRITE, 0);
    WHAL_ASSERT_EQ(Field(c, T_DUMMY_Pos, 0x1F), 0);
    /* Low byte of the lower data register first. */
    WHAL_ASSERT_EQ(id[0], Pat(0));
    WHAL_ASSERT_EQ(id[1], Pat(1));
    WHAL_ASSERT_EQ(id[2], Pat(2));
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_ASSERT_EQ(g_ncmd, 1);
#endif
}

static void Test_CadenceOspi_StigWriteRegisters(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint8_t w[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
    uint32_t c;

    Setup();
    WHAL_ASSERT_EQ(Run(&dev, 0x12, 4, 0, WHAL_SPIMEM_DIR_OUT, 0x1234560, w, 8),
                   WHAL_SUCCESS);
    c = LastCtrl();
    WHAL_ASSERT_EQ(Field(c, T_OPCODE_Pos, 0xFF), 0x12);
    WHAL_ASSERT_EQ((c & T_ENB_ADDR) != 0, 1);
    WHAL_ASSERT_EQ(Field(c, T_ADDR_BYTES_Pos, 3), 3);
    WHAL_ASSERT_EQ((c & T_ENB_WRITE) != 0, 1);
    WHAL_ASSERT_EQ(Field(c, T_WR_BYTES_Pos, 7), 7);
    WHAL_ASSERT_EQ(c & T_ENB_READ, 0);
    WHAL_ASSERT_EQ(REG(T_CMD_ADDR), 0x1234560);
    WHAL_ASSERT_EQ(REG(T_WR_LO), 0x44332211);
    WHAL_ASSERT_EQ(REG(T_WR_HI), 0x88776655);
}

static void Test_CadenceOspi_StigShortWriteKeepsUpper(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint8_t w[3] = { 0xA1, 0xB2, 0xC3 };
    uint32_t c;

    Setup();
    REG(T_WR_HI) = 0x5A5A5A5A;
    WHAL_ASSERT_EQ(Run(&dev, 0x02, 3, 0, WHAL_SPIMEM_DIR_OUT, 0x123456, w, 3),
                   WHAL_SUCCESS);
    c = LastCtrl();
    WHAL_ASSERT_EQ(Field(c, T_ADDR_BYTES_Pos, 3), 2);
    WHAL_ASSERT_EQ(Field(c, T_WR_BYTES_Pos, 7), 2);
    WHAL_ASSERT_EQ(REG(T_WR_LO), 0x00C3B2A1);
    WHAL_ASSERT_EQ(REG(T_WR_HI), 0x5A5A5A5A);
}

static void Test_CadenceOspi_StigNoData(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint32_t c;

    Setup();
    WHAL_ASSERT_EQ(Run(&dev, 0x06, 0, 0, WHAL_SPIMEM_DIR_NONE, 0, NULL, 0),
                   WHAL_SUCCESS);
    c = LastCtrl();
    WHAL_ASSERT_EQ(Field(c, T_OPCODE_Pos, 0xFF), 0x06);
    WHAL_ASSERT_EQ(c & (T_ENB_READ | T_ENB_WRITE | T_ENB_ADDR), 0);
}

/* The board's octal-DDR exit reads 9Fh with 8 dummy cycles and 8 bytes. */
static void Test_CadenceOspi_StigDummyRead8(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint8_t d[8];
    uint32_t c;
    int i;

    Setup();
    LoadReadData(0);
    WHAL_ASSERT_EQ(Run(&dev, 0x9F, 0, 8, WHAL_SPIMEM_DIR_IN, 0, d, 8),
                   WHAL_SUCCESS);
    c = LastCtrl();
    WHAL_ASSERT_EQ(Field(c, T_DUMMY_Pos, 0x1F), 8);
    WHAL_ASSERT_EQ(Field(c, T_RD_BYTES_Pos, 7), 7);
    WHAL_ASSERT_EQ(c & T_ENB_ADDR, 0);
    for (i = 0; i < 8; i++) {
        WHAL_ASSERT_EQ(d[i], Pat((uint32_t)i));
    }
}

static void Test_CadenceOspi_StigMaxDummy(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint8_t d[2];

    Setup();
    WHAL_ASSERT_EQ(Run(&dev, 0x9F, 0, 31, WHAL_SPIMEM_DIR_IN, 0, d, 2),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Field(LastCtrl(), T_DUMMY_Pos, 0x1F), 31);
}

/* --- Refusals: none of them may reach the controller --- */

static void Test_CadenceOspi_Refusals(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint8_t w[9];
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;

    Setup();
    memset(w, 0, sizeof(w));
    /* More than 31 dummy cycles. */
    WHAL_ASSERT_EQ(Run(&dev, 0x9F, 0, 32, WHAL_SPIMEM_DIR_IN, 0, w, 2),
                   WHAL_EINVAL);
    /* More than 4 address bytes. */
    WHAL_ASSERT_EQ(Run(&dev, 0x13, 5, 0, WHAL_SPIMEM_DIR_IN, 0x1000, w, 4),
                   WHAL_EINVAL);
    /* A write longer than one STIG command. */
    WHAL_ASSERT_EQ(Run(&dev, 0x12, 4, 0, WHAL_SPIMEM_DIR_OUT, 0x1000, w, 9),
                   WHAL_EINVAL);
    /* A read without an address longer than one STIG command. */
    WHAL_ASSERT_EQ(Run(&dev, 0x9F, 0, 0, WHAL_SPIMEM_DIR_IN, 0, w, 9),
                   WHAL_EINVAL);
    /* Data size with no buffer (also checked here, not only in the
     * dispatch layer). */
    op.opcode = 0x13;
    op.addrBytes = 4;
    op.dummyCycles = 0;
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.addr = 0x1000;
    op.data = NULL;
    op.dataSz = 4;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_CadenceOspi_SpiMem_Exec(&dev, &op), WHAL_EINVAL);
    /* Data with no direction, and a bad direction. */
    WHAL_ASSERT_EQ(Run(&dev, 0x06, 0, 0, WHAL_SPIMEM_DIR_NONE, 0, w, 1),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(Run(&dev, 0x06, 0, 0, 3, 0, w, 1), WHAL_EINVAL);
    /* NULL op and NULL device. */
    WHAL_ASSERT_EQ(whal_CadenceOspi_SpiMem_Exec(&dev, NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_CadenceOspi_SpiMem_Exec(NULL, &op), WHAL_EINVAL);

    WHAL_ASSERT_EQ(REG(T_CMD_CTRL), 0);
    WHAL_ASSERT_EQ(REG(T_CMD_ADDR), 0);
}

/* --- Reads with an address --- */

static void Test_CadenceOspi_ReadSplitIntoStig(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint8_t d[20];
    int i;

    NEED_EMU();
    Setup();
    WHAL_ASSERT_EQ(Run(&dev, 0x13, 4, 0, WHAL_SPIMEM_DIR_IN, 0x1001, d, 20),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_ncmd, 3);
    WHAL_ASSERT_EQ(g_addr[0], 0x1001);
    WHAL_ASSERT_EQ(g_addr[1], 0x1009);
    WHAL_ASSERT_EQ(g_addr[2], 0x1011);
    WHAL_ASSERT_EQ(Field(g_ctrl[0], T_RD_BYTES_Pos, 7), 7);
    WHAL_ASSERT_EQ(Field(g_ctrl[2], T_RD_BYTES_Pos, 7), 3);
    WHAL_ASSERT_EQ(Field(g_ctrl[2], T_ADDR_BYTES_Pos, 3), 3);
    for (i = 0; i < 20; i++) {
        WHAL_ASSERT_EQ(d[i], Pat(0x1001 + (uint32_t)i));
    }
}

/* A DMA hook but a flash address that is not 4-byte aligned: STIG only. */
static void Test_CadenceOspi_ReadUnalignedAddrUsesStig(void)
{
    whal_SpiMem dev = DEV_INIT;

    Setup();
    g_cfg.dma = &g_dma;
    WHAL_ASSERT_EQ(Run(&dev, 0x13, 4, 0, WHAL_SPIMEM_DIR_IN, 0x2001, g_buf,
                       128), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_armCalls, 0);
    WHAL_ASSERT_EQ(g_waitCalls, 0);
    WHAL_ASSERT_EQ(REG(T_CMD_ADDR), 0x2001 + 120);
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_ASSERT_EQ(g_ncmd, 16);
#endif
}

/* Whole buffer aligned: one DMA read and no STIG command. */
static void Test_CadenceOspi_ReadDmaWhole(void)
{
    whal_SpiMem dev = DEV_INIT;
    int i;

    Setup();
    g_cfg.dma = &g_dma;
    REG(T_DEV_INSTR_RD) = 0x1F000003;
    REG(T_IND_RD_WMARK) = 0x55;
    WHAL_ASSERT_EQ(Run(&dev, 0x13, 4, 0, WHAL_SPIMEM_DIR_IN, 0x4000, g_buf,
                       256), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(g_armCalls, 1);
    WHAL_ASSERT_EQ(g_waitCalls, 1);
    WHAL_ASSERT_EQ(g_armSeq < g_waitSeq, 1);
    WHAL_ASSERT_EQ(g_armBuf == g_buf, 1);
    WHAL_ASSERT_EQ(g_armLen, 256);
    WHAL_ASSERT_EQ(g_waitBuf == g_buf, 1);
    WHAL_ASSERT_EQ(g_waitLen, 256);
    /* Arm runs before the indirect read starts, Wait after. */
    WHAL_ASSERT_EQ(g_armIndCtrl, 0);
    WHAL_ASSERT_EQ(g_waitIndCtrl, T_IND_RD_START_BIT);
    /* The opcode is in DEV_INSTR_RD_CONFIG when Arm runs; the other
     * fields stay. */
    WHAL_ASSERT_EQ(g_armRdConfig, 0x1F000013);
    WHAL_ASSERT_EQ(REG(T_DEV_INSTR_RD), 0x1F000013);
    /* Indirect read registers. */
    WHAL_ASSERT_EQ((REG(T_CONFIG) & T_CFG_DMA_IF) != 0, 1);
    WHAL_ASSERT_EQ(REG(T_DMA_PERIPH), 0x602);
    WHAL_ASSERT_EQ(REG(T_SRAM_PARTITION), 0xFE);
    WHAL_ASSERT_EQ(REG(T_IND_RD_WMARK), 0);
    WHAL_ASSERT_EQ(REG(T_IND_AHB_TRIG), 0xC0000000);
    WHAL_ASSERT_EQ(REG(T_IND_TRIG_RANGE), 6);
    WHAL_ASSERT_EQ(REG(T_IND_RD_START), 0x4000);
    WHAL_ASSERT_EQ(REG(T_IND_RD_NUM), 256);
    /* No STIG command. */
    WHAL_ASSERT_EQ(REG(T_CMD_CTRL), 0);
    for (i = 0; i < 256; i++) {
        WHAL_ASSERT_EQ(g_buf[i], Pat(0x4000 + (uint32_t)i));
    }
}

/* Unaligned buffer: STIG head and tail, DMA for the aligned middle. */
static void Test_CadenceOspi_ReadDmaHeadTail(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint8_t *buf = g_buf + 4;   /* 4 bytes past an aligned line */
    int i;

    NEED_EMU();
    Setup();
    g_cfg.dma = &g_dma;
    WHAL_ASSERT_EQ(Run(&dev, 0x13, 4, 0, WHAL_SPIMEM_DIR_IN, 0x2000, buf,
                       300), WHAL_SUCCESS);
    /* Head 60 bytes (8 commands), middle 192 by DMA, tail 48 (6). */
    WHAL_ASSERT_EQ(g_armCalls, 1);
    WHAL_ASSERT_EQ(g_armBuf == g_buf + 64, 1);
    WHAL_ASSERT_EQ(g_armLen, 192);
    WHAL_ASSERT_EQ(REG(T_IND_RD_START), 0x2000 + 60);
    WHAL_ASSERT_EQ(REG(T_IND_RD_NUM), 192);
    WHAL_ASSERT_EQ(g_ncmd, 8 + 6);
    WHAL_ASSERT_EQ(g_addr[0], 0x2000);
    WHAL_ASSERT_EQ(Field(g_ctrl[7], T_RD_BYTES_Pos, 7), 3);
    WHAL_ASSERT_EQ(g_addr[7], 0x2000 + 56);
    WHAL_ASSERT_EQ(g_addr[8], 0x2000 + 252);
    WHAL_ASSERT_EQ(g_addr[13], 0x2000 + 292);
    for (i = 0; i < 300; i++) {
        WHAL_ASSERT_EQ(buf[i], Pat(0x2000 + (uint32_t)i));
    }
}

/* The op's address size differs from DEV_SIZE_CONFIG (set by Init). A DMA
 * read must send the op's address size; STIG for all of it is also fine. */
static void ReadDmaAddrBytes(uint32_t devSize, uint8_t addrBytes)
{
    whal_SpiMem dev = DEV_INIT;
    int i;

    Setup();
    g_cfg.dma = &g_dma;
    g_cfg.devSize = devSize;
    REG(T_DEV_SIZE) = devSize;
    WHAL_ASSERT_EQ(Run(&dev, 0x03, addrBytes, 0, WHAL_SPIMEM_DIR_IN, 0x4000,
                       g_buf, 256), WHAL_SUCCESS);
    if (g_waitCalls > 0) {
        WHAL_ASSERT_EQ(g_waitAddrBytes, addrBytes);
    }
    else {
        WHAL_ASSERT_EQ(g_ncmd, 256 / 8);
        WHAL_ASSERT_EQ(Field(g_ctrl[0], T_ADDR_BYTES_Pos, 3), addrBytes - 1);
    }
    for (i = 0; i < 256; i++) {
        WHAL_ASSERT_EQ(g_buf[i], Pat(0x4000 + (uint32_t)i));
    }
}

/* 3-byte op on the 4-byte VEK385 setup, and 4-byte op on a 3-byte setup. */
static void Test_CadenceOspi_ReadDmaAddrBytes(void)
{
    NEED_EMU();
    ReadDmaAddrBytes(WHAL_VERSAL2_OSPI_DEV_SIZE_VEK385, 3);
    ReadDmaAddrBytes((WHAL_VERSAL2_OSPI_DEV_SIZE_VEK385 & ~0xFUL) | 2UL, 4);
}

/* Too short for one aligned DMA block: STIG only. */
static void Test_CadenceOspi_ReadDmaTooShort(void)
{
    whal_SpiMem dev = DEV_INIT;

    Setup();
    g_cfg.dma = &g_dma;
    WHAL_ASSERT_EQ(Run(&dev, 0x13, 4, 0, WHAL_SPIMEM_DIR_IN, 0x2000, g_buf,
                       63), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_armCalls, 0);
    WHAL_ASSERT_EQ(REG(T_CMD_ADDR), 0x2000 + 56);
}

static void Test_CadenceOspi_ReadDmaWaitFails(void)
{
    whal_SpiMem dev = DEV_INIT;

    Setup();
    g_cfg.dma = &g_dma;
    g_waitRet = WHAL_EHARDWARE;
    WHAL_ASSERT_EQ(Run(&dev, 0x13, 4, 0, WHAL_SPIMEM_DIR_IN, 0x2000, g_buf,
                       256), WHAL_EHARDWARE);
    WHAL_ASSERT_EQ(g_armCalls, 1);
    WHAL_ASSERT_EQ(g_waitCalls, 1);
    /* No fallback to STIG; the indirect read is cancelled. */
    WHAL_ASSERT_EQ(REG(T_CMD_CTRL), 0);
    WHAL_ASSERT_EQ(REG(T_IND_RD_CTRL), T_IND_RD_CANCEL_BIT);
}

static void Test_CadenceOspi_ReadDmaArmFails(void)
{
    whal_SpiMem dev = DEV_INIT;

    Setup();
    g_cfg.dma = &g_dma;
    g_armRet = WHAL_ETIMEOUT;
    WHAL_ASSERT_EQ(Run(&dev, 0x13, 4, 0, WHAL_SPIMEM_DIR_IN, 0x2000, g_buf,
                       256), WHAL_ETIMEOUT);
    WHAL_ASSERT_EQ(g_armCalls, 1);
    WHAL_ASSERT_EQ(g_waitCalls, 0);
    /* The indirect read never started, and no STIG fallback. */
    WHAL_ASSERT_EQ(REG(T_IND_RD_CTRL), 0);
    WHAL_ASSERT_EQ(REG(T_CMD_CTRL), 0);
}

/* The DMA path has no dummy count, so a read with dummy cycles stays on
 * STIG. */
static void Test_CadenceOspi_ReadDummyUsesStig(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint32_t c;

    Setup();
    g_cfg.dma = &g_dma;
    WHAL_ASSERT_EQ(Run(&dev, 0x0C, 4, 8, WHAL_SPIMEM_DIR_IN, 0x2000, g_buf,
                       256), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_armCalls, 0);
    WHAL_ASSERT_EQ(g_waitCalls, 0);
    c = LastCtrl();
    WHAL_ASSERT_EQ(Field(c, T_OPCODE_Pos, 0xFF), 0x0C);
    WHAL_ASSERT_EQ(Field(c, T_DUMMY_Pos, 0x1F), 8);
    WHAL_ASSERT_EQ(REG(T_CMD_ADDR), 0x2000 + 248);
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_ASSERT_EQ(g_ncmd, 256 / 8);
#endif
}

/* A DMA hook with no Arm or Wait is a config error. */
static void Test_CadenceOspi_ReadDmaBadHook(void)
{
    whal_SpiMem dev = DEV_INIT;
    whal_CadenceOspi_SpiMem_Dma bad = g_dma;

    Setup();
    bad.Wait = NULL;
    g_cfg.dma = &bad;
    WHAL_ASSERT_EQ(Run(&dev, 0x13, 4, 0, WHAL_SPIMEM_DIR_IN, 0x2000, g_buf,
                       256), WHAL_EINVAL);
    WHAL_ASSERT_EQ(REG(T_CMD_CTRL), 0);
}

/* --- Waits --- */

static void Test_CadenceOspi_WaitsForIdle(void)
{
    whal_SpiMem dev = DEV_INIT;
    uint8_t w[4] = { 1, 2, 3, 4 };
    uint8_t d[8];
    int i;

    NEED_EMU();
    Setup();
    /* Emulator check: a command sent while not idle loses its data. */
    REG(T_CONFIG) &= ~(uint32_t)T_CFG_IDLE;
    REG(T_CMD_CTRL) = T_EXEC;
    Emu_Step();
    WHAL_ASSERT_EQ(g_wrDataLost, 1);

    Setup();
    /* The done bit clears after 2 polls, CONFIG idle 5 polls later. */
    g_statusPolls = 2;
    g_idlePolls = 5;
    WHAL_ASSERT_EQ(Run(&dev, 0x06, 0, 0, WHAL_SPIMEM_DIR_NONE, 0, NULL, 0),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ((REG(T_CONFIG) & T_CFG_IDLE) != 0, 1);
    WHAL_ASSERT_EQ(g_steps >= 2 + 5, 1);
    WHAL_ASSERT_EQ(Run(&dev, 0x12, 4, 0, WHAL_SPIMEM_DIR_OUT, 0x3000, w, 4),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Run(&dev, 0x13, 4, 0, WHAL_SPIMEM_DIR_IN, 0x3000, d, 8),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_ncmd, 3);
    WHAL_ASSERT_EQ(g_wrDataLost, 0);
    WHAL_ASSERT_EQ((uint32_t)g_wdata[1], 0x04030201);
    /* Read data is taken after the command is done. */
    for (i = 0; i < 8; i++) {
        WHAL_ASSERT_EQ(d[i], Pat(0x3000 + (uint32_t)i));
    }
}

static void Test_CadenceOspi_StigTimeout(void)
{
    whal_SpiMem dev = DEV_INIT;

    NEED_EMU();
    Setup();
    g_stuck = 1;
    WHAL_ASSERT_EQ(Run(&dev, 0x06, 0, 0, WHAL_SPIMEM_DIR_NONE, 0, NULL, 0),
                   WHAL_ETIMEOUT);
    WHAL_ASSERT_EQ(g_ncmd, 1);
}

/* --- MaxDataSz --- */

static void Test_CadenceOspi_MaxDataSz(void)
{
    whal_SpiMem dev = DEV_INIT;
    whal_SpiMemOp op;
    size_t sz;

    Setup();
    memset(&op, 0, sizeof(op));

    op.dir = WHAL_SPIMEM_DIR_IN;
    op.addrBytes = 4;
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&dev, &op, &sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(sz == SIZE_MAX, 1);

    op.addrBytes = 0;
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&dev, &op, &sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(sz, 8);

    op.dir = WHAL_SPIMEM_DIR_OUT;
    op.addrBytes = 4;
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&dev, &op, &sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(sz, 8);

    op.dir = WHAL_SPIMEM_DIR_NONE;
    WHAL_ASSERT_EQ(whal_SpiMem_MaxDataSz(&dev, &op, &sz), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(sz, 8);

    WHAL_ASSERT_EQ(whal_CadenceOspi_SpiMem_MaxDataSz(&dev, NULL, &sz),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_CadenceOspi_SpiMem_MaxDataSz(&dev, &op, NULL),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_CadenceOspi_SpiMem_MaxDataSz(NULL, &op, &sz),
                   WHAL_EINVAL);
}

/* --- Bus mode --- */

/* An op with an octal DTR bus mode sets the instruction registers and
 * CONFIG DTR; a 1-bit op after it sets them back. */
static void Test_CadenceOspi_BusModeOctalDtr(void)
{
    whal_SpiMem dev = DEV_INIT;
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    uint8_t id[8];

    Setup();
    op.opcode = 0x9F;
    op.opLines = 8;
    op.dataLines = 8;
    op.dtr = 1;
    op.dummyCycles = 8;
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.data = id;
    op.dataSz = sizeof(id);
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_SUCCESS);
    /* Instruction and data on 8 lines, no address (1 line). */
    WHAL_ASSERT_EQ(REG(T_DEV_INSTR_RD),
                   0x3 | (3UL << T_RD_INSTR_Pos) | (3UL << T_RD_DATA_Pos));
    WHAL_ASSERT_EQ(REG(T_CONFIG) & (T_CFG_DTR | T_CFG_ENB_SPI),
                   T_CFG_DTR | T_CFG_ENB_SPI);
    WHAL_ASSERT_EQ(Field(LastCtrl(), T_DUMMY_Pos, 0x1F), 8);

    WHAL_ASSERT_EQ(Run(&dev, 0x70, 0, 0, WHAL_SPIMEM_DIR_IN, 0, id, 1),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(REG(T_DEV_INSTR_RD), 0x3);
    WHAL_ASSERT_EQ(REG(T_CONFIG), T_CONFIG_RESET);
}

/* Address and data lines also go into DEV_INSTR_WR_CONFIG. */
static void Test_CadenceOspi_BusModeWriteLines(void)
{
    whal_SpiMem dev = DEV_INIT;
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    uint8_t w[4] = { 1, 2, 3, 4 };

    Setup();
    REG(T_DEV_INSTR_WR) = 0x2;
    op.opcode = 0x12;
    op.opLines = 1;
    op.addrLines = 4;
    op.dataLines = 2;
    op.addrBytes = 4;
    op.addr = 0x1000;
    op.dir = WHAL_SPIMEM_DIR_OUT;
    op.data = w;
    op.dataSz = sizeof(w);
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(REG(T_DEV_INSTR_RD),
                   0x3 | (2UL << T_RD_ADDR_Pos) | (1UL << T_RD_DATA_Pos));
    WHAL_ASSERT_EQ(REG(T_DEV_INSTR_WR),
                   0x2 | (2UL << T_WR_ADDR_Pos) | (1UL << T_WR_DATA_Pos));
    WHAL_ASSERT_EQ(REG(T_CONFIG) & T_CFG_DTR, 0);
}

/* Bad line counts and odd DTR reads never reach the controller. A phase
 * the op does not have may have any line count. */
static void Test_CadenceOspi_BusModeRefusals(void)
{
    whal_SpiMem dev = DEV_INIT;
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;
    uint8_t r[8];

    Setup();
    op.opcode = 0x9F;
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.data = r;
    op.dataSz = 2;
    op.opLines = 3;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_EINVAL);
    op.opLines = 0;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_EINVAL);
    op.opLines = 1;
    op.dataLines = 16;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_EINVAL);
    op.dataLines = 8;
    op.dtr = 1;
    op.dataSz = 3;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_EINVAL);
    op.addrBytes = 4;
    op.addrLines = 0;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_EINVAL);
    WHAL_ASSERT_EQ(REG(T_CMD_CTRL), 0);
    WHAL_ASSERT_EQ(REG(T_DEV_INSTR_RD), 0x3);
    WHAL_ASSERT_EQ(REG(T_CONFIG), T_CONFIG_RESET);

    /* No address and no data: their line counts are not used. */
    op.addrBytes = 0;
    op.dir = WHAL_SPIMEM_DIR_NONE;
    op.data = NULL;
    op.dataSz = 0;
    op.dtr = 0;
    op.dataLines = 0;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_SUCCESS);
}

/* A DTR read with an address goes through STIG, not the DMA. */
static void Test_CadenceOspi_ReadDtrUsesStig(void)
{
    whal_SpiMem dev = DEV_INIT;
    whal_SpiMemOp op = WHAL_SPIMEM_OP_INIT_1BIT;

    Setup();
    g_cfg.dma = &g_dma;
    op.opcode = 0xFD;
    op.opLines = 8;
    op.addrLines = 8;
    op.dataLines = 8;
    op.dtr = 1;
    op.addrBytes = 4;
    op.addr = 0x2000;
    op.dir = WHAL_SPIMEM_DIR_IN;
    op.data = g_buf;
    op.dataSz = 256;
    WHAL_ASSERT_EQ(whal_SpiMem_Exec(&dev, &op), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_armCalls, 0);
    WHAL_ASSERT_EQ(g_waitCalls, 0);
    WHAL_ASSERT_EQ(REG(T_CMD_ADDR), 0x2000 + 248);
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_ASSERT_EQ(g_ncmd, 256 / 8);
#endif
}

/* --- Init / Deinit --- */

/* Init over the VEK385 registers after an OSPI boot: config applied, DTR,
 * PHY, pipeline PHY and dual opcode off, instruction registers 1-bit. */
static void Test_CadenceOspi_InitSetsConfig(void)
{
    whal_SpiMem dev = DEV_INIT;

    Setup();
    REG(T_CONFIG) = 0x8100BC49 | T_CFG_PIPELINE_PHY | T_CFG_DUAL_OPCODE;
    REG(T_DEV_INSTR_RD) = 0x1103337C;
    REG(T_DEV_INSTR_WR) = 0x00033302;
    REG(T_RD_CAPTURE) = 0x101;
    g_cfg.baudDiv = 0xF;
    g_cfg.csLines = 0;
    g_cfg.rdCapture = 0x1;
    WHAL_ASSERT_EQ(whal_SpiMem_Init(&dev), WHAL_SUCCESS);
    /* The value that read the flash in 1-bit on the VEK385. */
    WHAL_ASSERT_EQ(REG(T_CONFIG), 0x80788041);
    WHAL_ASSERT_EQ(REG(T_RD_CAPTURE), 0x1);
    WHAL_ASSERT_EQ(REG(T_DEV_INSTR_RD), 0x3);
    WHAL_ASSERT_EQ(REG(T_DEV_INSTR_WR), 0x2);
    WHAL_ASSERT_EQ(REG(T_DEV_SIZE), 0x00501003);

    /* Other values: /6 clock, chip select lines 0xE. */
    Setup();
    g_cfg.baudDiv = 2;
    g_cfg.csLines = 0xE;
    g_cfg.rdCapture = 0x31;
    WHAL_ASSERT_EQ(whal_SpiMem_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(REG(T_CONFIG),
                   (T_CONFIG_RESET & ~(uint32_t)T_CFG_BAUD_Msk) |
                   (2UL << T_CFG_BAUD_Pos) | (0xEUL << T_CFG_CS_Pos));
    WHAL_ASSERT_EQ(REG(T_RD_CAPTURE), 0x31);

    /* A field value too wide is masked to the field. */
    Setup();
    g_cfg.baudDiv = 0x1F;
    WHAL_ASSERT_EQ(whal_SpiMem_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(REG(T_CONFIG), T_CONFIG_RESET);
}

static void Test_CadenceOspi_InitEnables(void)
{
    whal_SpiMem dev = DEV_INIT;

    Setup();
    g_cfg.baudDiv = 0xF;
    REG(T_CONFIG) = T_CONFIG_RESET & ~(uint32_t)T_CFG_ENB_SPI;
    WHAL_ASSERT_EQ(whal_SpiMem_Init(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(REG(T_CONFIG), T_CONFIG_RESET);
}

static void Test_CadenceOspi_InitDeinitArgs(void)
{
    whal_SpiMem noCfg = {
        .base = (size_t)(uintptr_t)g_regs,
        .driver = &whal_CadenceOspi_SpiMem_Driver,
        .cfg = NULL,
    };

    Setup();
    WHAL_ASSERT_EQ(whal_CadenceOspi_SpiMem_Init(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_CadenceOspi_SpiMem_Init(&noCfg), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_CadenceOspi_SpiMem_Deinit(NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_CadenceOspi_SpiMem_Exec(&noCfg, NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(REG(T_CONFIG), T_CONFIG_RESET);
}

static void Test_CadenceOspi_Deinit(void)
{
    whal_SpiMem dev = DEV_INIT;

    Setup();
    REG(T_CONFIG) = T_CONFIG_RESET | T_CFG_DMA_IF;
    WHAL_ASSERT_EQ(whal_SpiMem_Deinit(&dev), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(REG(T_CONFIG),
                   (T_CONFIG_RESET | T_CFG_DMA_IF) & ~(uint32_t)T_CFG_ENB_SPI);
}

void whal_Test_CadenceOspi_SpiMem(void)
{
    WHAL_TEST_SUITE_START("cadence_ospi_spimem");
    WHAL_TEST(Test_CadenceOspi_StigReadRegisters);
    WHAL_TEST(Test_CadenceOspi_StigWriteRegisters);
    WHAL_TEST(Test_CadenceOspi_StigShortWriteKeepsUpper);
    WHAL_TEST(Test_CadenceOspi_StigNoData);
    WHAL_TEST(Test_CadenceOspi_StigDummyRead8);
    WHAL_TEST(Test_CadenceOspi_StigMaxDummy);
    WHAL_TEST(Test_CadenceOspi_Refusals);
    WHAL_TEST(Test_CadenceOspi_ReadSplitIntoStig);
    WHAL_TEST(Test_CadenceOspi_ReadUnalignedAddrUsesStig);
    WHAL_TEST(Test_CadenceOspi_ReadDmaWhole);
    WHAL_TEST(Test_CadenceOspi_ReadDmaHeadTail);
    WHAL_TEST(Test_CadenceOspi_ReadDmaAddrBytes);
    WHAL_TEST(Test_CadenceOspi_ReadDmaTooShort);
    WHAL_TEST(Test_CadenceOspi_ReadDmaWaitFails);
    WHAL_TEST(Test_CadenceOspi_ReadDmaArmFails);
    WHAL_TEST(Test_CadenceOspi_ReadDummyUsesStig);
    WHAL_TEST(Test_CadenceOspi_ReadDmaBadHook);
    WHAL_TEST(Test_CadenceOspi_WaitsForIdle);
    WHAL_TEST(Test_CadenceOspi_StigTimeout);
    WHAL_TEST(Test_CadenceOspi_MaxDataSz);
    WHAL_TEST(Test_CadenceOspi_BusModeOctalDtr);
    WHAL_TEST(Test_CadenceOspi_BusModeWriteLines);
    WHAL_TEST(Test_CadenceOspi_BusModeRefusals);
    WHAL_TEST(Test_CadenceOspi_ReadDtrUsesStig);
    WHAL_TEST(Test_CadenceOspi_InitSetsConfig);
    WHAL_TEST(Test_CadenceOspi_InitEnables);
    WHAL_TEST(Test_CadenceOspi_InitDeinitArgs);
    WHAL_TEST(Test_CadenceOspi_Deinit);
    WHAL_TEST_SUITE_END();
}
