/* test_stm32h5_eth.c
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
#include <wolfHAL/eth/stm32h5_eth.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * STM32H5 Ethernet platform-specific tests.
 *
 * Uses MAC-internal loopback to test the full send/recv path without
 * requiring a cable or link partner.
 */

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
    whal_Error err;

    WHAL_ASSERT_EQ(whal_Stm32h5_Eth_Ext_EnableLoopback(BOARD_ETH_DEV, 1),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Eth_Start(BOARD_ETH_DEV, WHAL_ETH_SPEED_100,
                                  WHAL_ETH_DUPLEX_FULL), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Eth_Send(BOARD_ETH_DEV, txFrame, sizeof(txFrame)),
                   WHAL_SUCCESS);

    /* Poll for the looped-back frame */
    uint32_t start = g_tick;
    do {
        err = whal_Eth_Recv(BOARD_ETH_DEV, rxFrame, &rxLen);
    } while (err == WHAL_ENOTREADY && (g_tick - start) < 100);

    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);

    /* Verify payload matches (skip MAC header) */
    WHAL_ASSERT_MEM_EQ(rxFrame + 14, txFrame + 14, sizeof(txFrame) - 14);

    WHAL_ASSERT_EQ(whal_Eth_Stop(BOARD_ETH_DEV), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Stm32h5_Eth_Ext_EnableLoopback(BOARD_ETH_DEV, 0),
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

    WHAL_ASSERT_EQ(whal_Stm32h5_Eth_Ext_EnableLoopback(BOARD_ETH_DEV, 1),
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
    WHAL_ASSERT_EQ(whal_Stm32h5_Eth_Ext_EnableLoopback(BOARD_ETH_DEV, 0),
                   WHAL_SUCCESS);
}

void whal_Test_Eth_Platform(void)
{
    WHAL_TEST_SUITE_START("eth_platform");
    WHAL_TEST(Test_Eth_Loopback);
    WHAL_TEST(Test_Eth_UndersizedRecv);
    WHAL_TEST_SUITE_END();
}
