/* test_ieee802154.c
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
#include <stdbool.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/ieee802154/ieee802154.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * Generic IEEE 802.15.4 radio tests.
 *
 * Runs on a single board: transmit results are checked through TxConfirm, and
 * the frame requesting an ACK goes to an address no device should answer.
 */

#define TEST_CHANNEL        15
#define TEST_PAN_ID         0xBEEF
#define TEST_SHORT_ADDR     0x1234
#define TEST_NO_ACK_ADDR    0xFFFE
#define TEST_WAIT_MS        1000
#define TEST_ED_US          1024

typedef struct {
    volatile bool txDone;
    volatile whal_Ieee802154_TxStatus txStatus;
    volatile bool edDone;
    volatile int8_t edLevel;
} TestEvents;

static TestEvents g_events;

static void Test_TxConfirm(void *ctx, whal_Ieee802154_TxStatus status,
                           const uint8_t *ack, size_t ackLen)
{
    TestEvents *ev = ctx;
    (void)ack;
    (void)ackLen;

    ev->txStatus = status;
    ev->txDone = true;
}

static void Test_EdConfirm(void *ctx, int8_t maxRssi)
{
    TestEvents *ev = ctx;

    ev->edLevel = maxRssi;
    ev->edDone = true;
}

static const whal_Ieee802154_Callbacks g_testCb = {
    .TxConfirm = Test_TxConfirm,
    .EdConfirm = Test_EdConfirm,
};

static bool Test_WaitFlag(volatile bool *flag)
{
    for (int i = 0; i < TEST_WAIT_MS && !*flag; i++) {
        Board_WaitMs(1);
    }
    return *flag;
}

static void Test_Ieee802154_Api(void)
{
    const whal_Ieee802154_TxCfg txCfg = { 0 };
    whal_Ieee802154_AttrVal val;
    uint8_t frame[WHAL_IEEE802154_FRAME_MAX + 1] = { 0 };
    uint32_t caps;

    WHAL_ASSERT_EQ(whal_Ieee802154_GetCaps(BOARD_IEEE802154_DEV, NULL),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Ieee802154_GetCaps(NULL, &caps), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_CHANNEL, NULL),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_CHANNEL, NULL),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, NULL, 10,
                                            &txCfg), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame, 10,
                                            NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame, 0,
                                            &txCfg), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame,
                                            sizeof(frame), &txCfg),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Ieee802154_GetRssi(BOARD_IEEE802154_DEV, NULL),
                   WHAL_EINVAL);

    val.bytes[0] = 0;
    WHAL_ASSERT_EQ(whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_EUI64, &val),
                   WHAL_EINVAL);
}

static void Test_Ieee802154_AttrRoundTrip(void)
{
    static const uint8_t extAddr[WHAL_IEEE802154_EXT_ADDR_SZ] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    };
    whal_Ieee802154_AttrVal val;

    val.u8 = TEST_CHANNEL;
    WHAL_ASSERT_EQ(whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_CHANNEL, &val),
                   WHAL_SUCCESS);
    val.u16 = TEST_PAN_ID;
    WHAL_ASSERT_EQ(whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_PAN_ID, &val),
                   WHAL_SUCCESS);
    val.u16 = TEST_SHORT_ADDR;
    WHAL_ASSERT_EQ(whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_SHORT_ADDR,
                                           &val), WHAL_SUCCESS);
    for (size_t i = 0; i < sizeof(extAddr); i++) {
        val.bytes[i] = extAddr[i];
    }
    WHAL_ASSERT_EQ(whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_EXT_ADDR, &val),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_CHANNEL, &val),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val.u8, TEST_CHANNEL);
    WHAL_ASSERT_EQ(whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_PAN_ID, &val),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val.u16, TEST_PAN_ID);
    WHAL_ASSERT_EQ(whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_SHORT_ADDR,
                                           &val), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val.u16, TEST_SHORT_ADDR);
    WHAL_ASSERT_EQ(whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_EXT_ADDR, &val),
                   WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(val.bytes, extAddr, sizeof(extAddr));
}

static void Test_Ieee802154_Eui64(void)
{
    whal_Ieee802154_AttrVal val;
    bool allZero = true;
    bool allOnes = true;

    WHAL_ASSERT_EQ(whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_EUI64, &val),
                   WHAL_SUCCESS);

    for (size_t i = 0; i < WHAL_IEEE802154_EXT_ADDR_SZ; i++) {
        allZero &= val.bytes[i] == 0x00;
        allOnes &= val.bytes[i] == 0xFF;
    }
    WHAL_ASSERT_EQ(allZero, false);
    WHAL_ASSERT_EQ(allOnes, false);
}

/* Data frame, short addressing, PAN ID compression: FCF, sequence number,
 * destination PAN, destination and source short address, payload. */
static size_t Test_BuildFrame(uint8_t *frame, uint16_t dst, bool ackRequest)
{
    const uint8_t payload[] = { 'w', 'o', 'l', 'f' };
    size_t len = 0;
    uint16_t fcf = 0x0001 | 0x0040 | (2u << 10) | (2u << 14);

    if (ackRequest) {
        fcf |= 0x0020;
    }

    frame[len++] = (uint8_t)fcf;
    frame[len++] = (uint8_t)(fcf >> 8);
    frame[len++] = 0x5A;
    frame[len++] = (uint8_t)TEST_PAN_ID;
    frame[len++] = (uint8_t)(TEST_PAN_ID >> 8);
    frame[len++] = (uint8_t)dst;
    frame[len++] = (uint8_t)(dst >> 8);
    frame[len++] = (uint8_t)TEST_SHORT_ADDR;
    frame[len++] = (uint8_t)(TEST_SHORT_ADDR >> 8);
    for (size_t i = 0; i < sizeof(payload); i++) {
        frame[len++] = payload[i];
    }

    return len;
}

static void Test_Ieee802154_TransmitBroadcast(void)
{
    const whal_Ieee802154_TxCfg txCfg = {
        .csmaCa = true,
        .maxCsmaBackoffs = 4,
        .minBe = 3,
        .maxBe = 5,
        .maxFrameRetries = 0,
    };
    uint8_t frame[32];
    size_t len = Test_BuildFrame(frame, 0xFFFF, false);

    g_events.txDone = false;
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame, len,
                                            &txCfg), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Test_WaitFlag(&g_events.txDone), true);
    WHAL_ASSERT_EQ(g_events.txStatus, WHAL_IEEE802154_TX_SUCCESS);
}

static void Test_Ieee802154_TransmitNoAck(void)
{
    const whal_Ieee802154_TxCfg txCfg = {
        .csmaCa = true,
        .maxCsmaBackoffs = 4,
        .minBe = 3,
        .maxBe = 5,
        .maxFrameRetries = 3,
    };
    uint8_t frame[32];
    size_t len = Test_BuildFrame(frame, TEST_NO_ACK_ADDR, true);

    g_events.txDone = false;
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame, len,
                                            &txCfg), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Test_WaitFlag(&g_events.txDone), true);
    WHAL_ASSERT_EQ(g_events.txStatus, WHAL_IEEE802154_TX_NO_ACK);
}

static void Test_Ieee802154_EnergyDetect(void)
{
    uint32_t caps = 0;

    WHAL_ASSERT_EQ(whal_Ieee802154_GetCaps(BOARD_IEEE802154_DEV, &caps),
                   WHAL_SUCCESS);
    if (!(caps & WHAL_IEEE802154_CAP_ENERGY_DETECT)) {
        WHAL_SKIP();
    }

    g_events.edDone = false;
    WHAL_ASSERT_EQ(whal_Ieee802154_EnergyDetect(BOARD_IEEE802154_DEV,
                                                TEST_ED_US), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(Test_WaitFlag(&g_events.edDone), true);

    whal_Test_Printf("  channel %d energy: %d dBm\n", TEST_CHANNEL,
                     g_events.edLevel);
    WHAL_ASSERT_EQ(g_events.edLevel < 0, true);
}

void whal_Test_Ieee802154(void)
{
    Board_Ieee802154SetCallbacks(&g_testCb, &g_events);

    WHAL_TEST_SUITE_START("ieee802154");
    WHAL_TEST(Test_Ieee802154_Api);
    WHAL_TEST(Test_Ieee802154_AttrRoundTrip);
    WHAL_TEST(Test_Ieee802154_Eui64);
    WHAL_TEST(Test_Ieee802154_TransmitBroadcast);
    WHAL_TEST(Test_Ieee802154_TransmitNoAck);
    WHAL_TEST(Test_Ieee802154_EnergyDetect);
    WHAL_TEST_SUITE_END();

    Board_Ieee802154SetCallbacks(NULL, NULL);
}
