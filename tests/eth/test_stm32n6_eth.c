/* test_stm32n6_eth.c
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
#include <wolfHAL/eth/eth.h>
#include <wolfHAL/eth/stm32n6_eth.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * STM32N6 Ethernet platform-specific tests.
 *
 * Uses MAC-internal loopback (MACCR.LM) to test the full send/recv path
 * without requiring a cable or link partner. Per RM0486 §76.5.10 the PHY
 * must still be providing eth_rx_clk for the loopback FIFO to drain, so
 * Board_Init must have brought the LAN8742A out of power-down before this
 * test runs.
 */

/* MAC and DMA register offsets we want to read for diagnostics */
#define DBG_MACCR_REG     0x0000
#define DBG_DMAC0SR_REG   0x1160
#define DBG_DMADSR_REG    0x100C
#define DBG_MACDR_REG     0x0114 /* MAC debug register */

/* Print a 32-bit value as two 16-bit halves (whal_Test_Printf only does %d). */
static void DumpHex32(const char *label, uint32_t v)
{
    whal_Test_Printf("  %s: hi=%d lo=%d\n", label,
                     (int)((v >> 16) & 0xFFFF), (int)(v & 0xFFFF));
}

static void Test_Eth_Loopback(void)
{
    uint8_t txFrame[64] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x80, 0xE1, 0x00, 0x00, 0x01,
        0x08, 0x00,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD,
    };
    uint8_t rxFrame[1536];
    size_t rxLen = sizeof(rxFrame);
    size_t base = whal_Stm32n6_Eth_Dev.base;
    whal_Stm32n6_Eth_Cfg *ethCfg = (whal_Stm32n6_Eth_Cfg *)whal_Stm32n6_Eth_Dev.cfg;
    whal_Error err;
    uint16_t bcr = 0;

    /* Verify the LAN8742A is alive and not in software power-down. */
    err = whal_Eth_MdioRead(BOARD_ETH_DEV, BOARD_ETH_PHY_ADDR, 0x00, &bcr);
    whal_Test_Printf("  PHY BCR read: err=%d val=%d (pwd=%d)\n",
                     (int)err, (int)bcr, (int)((bcr >> 11) & 1));

    DumpHex32("txDescs addr", (uint32_t)(uintptr_t)ethCfg->txDescs);
    DumpHex32("rxDescs addr", (uint32_t)(uintptr_t)ethCfg->rxDescs);
    DumpHex32("txBufs  addr", (uint32_t)(uintptr_t)ethCfg->txBufs);
    DumpHex32("rxBufs  addr", (uint32_t)(uintptr_t)ethCfg->rxBufs);

    /* Read back RIFSC_RIMC_ATTR6 (secure alias). If 0, our write didn't
     * take and the CPU is probably non-secure. */
    DumpHex32("RIMC_ATTR6 sec",
              *(volatile uint32_t *)(0x54024000U + 0xC28U));
    DumpHex32("RIMC_ATTR6 nse",
              *(volatile uint32_t *)(0x44024000U + 0xC28U));

    /* DMA channel state we expect to be running after Eth_Start. */
    DumpHex32("DMAC0TXCR (pre)", (uint32_t)whal_Reg_Read(base, 0x1104));
    DumpHex32("DMAC0RXCR (pre)", (uint32_t)whal_Reg_Read(base, 0x1108));
    DumpHex32("DMAC0TXDLAR",     (uint32_t)whal_Reg_Read(base, 0x1114));
    DumpHex32("DMAC0RXDLAR",     (uint32_t)whal_Reg_Read(base, 0x111C));

    WHAL_ASSERT_EQ(whal_Stm32n6_Eth_Ext_EnableLoopback(BOARD_ETH_DEV, 1),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Eth_Start(BOARD_ETH_DEV, WHAL_ETH_SPEED_100,
                                  WHAL_ETH_DUPLEX_FULL), WHAL_SUCCESS);

    DumpHex32("MACCR after Start", (uint32_t)whal_Reg_Read(base,
                                                           DBG_MACCR_REG));

    WHAL_ASSERT_EQ(whal_Eth_Send(BOARD_ETH_DEV, txFrame, sizeof(txFrame)),
                   WHAL_SUCCESS);

    /* Give the MAC a moment, then snapshot TX/RX descriptor state. */
    Board_WaitMs(20);
    DumpHex32("TDES3 after Send+20ms",   ethCfg->txDescs[0].des[3]);
    DumpHex32("RDES3 after Send+20ms",   ethCfg->rxDescs[0].des[3]);
    DumpHex32("DMADSR",                   (uint32_t)whal_Reg_Read(base,
                                                              DBG_DMADSR_REG));
    DumpHex32("DMAC0SR",                  (uint32_t)whal_Reg_Read(base,
                                                              DBG_DMAC0SR_REG));

    /* Poll for the looped-back frame */
    uint32_t start = g_tick;
    do {
        err = whal_Eth_Recv(BOARD_ETH_DEV, rxFrame, &rxLen);
    } while (err == WHAL_ENOTREADY && (g_tick - start) < 100);

    if (err != WHAL_SUCCESS) {
        DumpHex32("MACCR final",  (uint32_t)whal_Reg_Read(base, DBG_MACCR_REG));
        DumpHex32("DMADSR final", (uint32_t)whal_Reg_Read(base,
                                                          DBG_DMADSR_REG));
        DumpHex32("DMAC0SR final",(uint32_t)whal_Reg_Read(base,
                                                          DBG_DMAC0SR_REG));
        DumpHex32("TDES3 final",  ethCfg->txDescs[0].des[3]);
        DumpHex32("RDES3 final",  ethCfg->rxDescs[0].des[3]);
    }

    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);

    /* Verify payload matches (skip MAC header) */
    WHAL_ASSERT_MEM_EQ(rxFrame + 14, txFrame + 14, sizeof(txFrame) - 14);

    WHAL_ASSERT_EQ(whal_Eth_Stop(BOARD_ETH_DEV), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Stm32n6_Eth_Ext_EnableLoopback(BOARD_ETH_DEV, 0),
                   WHAL_SUCCESS);
}

static void Test_Eth_UndersizedRecv(void)
{
    uint8_t txFrame[64] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x80, 0xE1, 0x00, 0x00, 0x01,
        0x08, 0x00,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
        0xDE, 0xAD,
    };
    uint8_t smallBuf[32];
    uint8_t rxFrame[1536];
    size_t smallLen;
    size_t rxLen = sizeof(rxFrame);
    whal_Error err;
    uint32_t start;

    WHAL_ASSERT_EQ(whal_Stm32n6_Eth_Ext_EnableLoopback(BOARD_ETH_DEV, 1),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Eth_Start(BOARD_ETH_DEV, WHAL_ETH_SPEED_100,
                                  WHAL_ETH_DUPLEX_FULL), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Eth_Send(BOARD_ETH_DEV, txFrame, sizeof(txFrame)),
                   WHAL_SUCCESS);

    start = g_tick;
    do {
        smallLen = sizeof(smallBuf);
        err = whal_Eth_Recv(BOARD_ETH_DEV, smallBuf, &smallLen);
    } while (err == WHAL_ENOTREADY && (g_tick - start) < 100);

    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    WHAL_ASSERT_EQ(smallLen > sizeof(smallBuf), 1);

    err = whal_Eth_Recv(BOARD_ETH_DEV, rxFrame, &rxLen);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(rxLen, smallLen);
    WHAL_ASSERT_MEM_EQ(rxFrame + 14, txFrame + 14, sizeof(txFrame) - 14);

    WHAL_ASSERT_EQ(whal_Eth_Stop(BOARD_ETH_DEV), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Stm32n6_Eth_Ext_EnableLoopback(BOARD_ETH_DEV, 0),
                   WHAL_SUCCESS);
}

void whal_Test_Eth_Platform(void)
{
    WHAL_TEST_SUITE_START("eth_platform");
    WHAL_TEST(Test_Eth_Loopback);
    WHAL_TEST(Test_Eth_UndersizedRecv);
    WHAL_TEST_SUITE_END();
}
