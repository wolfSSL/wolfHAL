/* test_esp32c6_ieee802154.c
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

#define PENDING_TABLE_SZ    16
#define CHANNEL_MIN         11
#define CHANNEL_MAX         26
#define RX_SENSITIVITY_DBM  (-104)
#define SYMBOL_US           16

#define EXPECTED_CAPS (WHAL_IEEE802154_CAP_AUTO_ACK | \
                       WHAL_IEEE802154_CAP_ACK_WAIT | \
                       WHAL_IEEE802154_CAP_CSMA_CA | \
                       WHAL_IEEE802154_CAP_RETRANSMIT | \
                       WHAL_IEEE802154_CAP_FRAME_FILTER | \
                       WHAL_IEEE802154_CAP_PROMISCUOUS | \
                       WHAL_IEEE802154_CAP_PENDING_TABLE | \
                       WHAL_IEEE802154_CAP_ENERGY_DETECT)

static void ExtAddr(uint8_t *addr, size_t n)
{
    for (size_t i = 0; i < WHAL_IEEE802154_EXT_ADDR_SZ; i++) {
        addr[i] = (uint8_t)(0xA0 + i);
    }
    addr[WHAL_IEEE802154_EXT_ADDR_SZ - 1] = (uint8_t)n;
}

static whal_Error SetU8(whal_Ieee802154_Attr attr, uint8_t v)
{
    whal_Ieee802154_AttrVal val = { .u8 = v };

    return whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV, attr, &val);
}

static void Test_Esp32c6_Ieee802154_Caps(void)
{
    uint32_t caps = 0;

    WHAL_ASSERT_EQ(whal_Ieee802154_GetCaps(BOARD_IEEE802154_DEV, &caps),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(caps, EXPECTED_CAPS);
}

static void Test_Esp32c6_Ieee802154_PendingTableShortFull(void)
{
    uint8_t ext[WHAL_IEEE802154_EXT_ADDR_SZ];

    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableClear(BOARD_IEEE802154_DEV),
                   WHAL_SUCCESS);

    for (uint16_t i = 0; i < PENDING_TABLE_SZ; i++) {
        WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddShort(
                           BOARD_IEEE802154_DEV, 0x1000 + i), WHAL_SUCCESS);
    }
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddShort(
                       BOARD_IEEE802154_DEV, 0x1000 + PENDING_TABLE_SZ),
                   WHAL_ENOTREADY);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddShort(BOARD_IEEE802154_DEV,
                                                        0x1003), WHAL_SUCCESS);

    ExtAddr(ext, 0);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddExtended(BOARD_IEEE802154_DEV,
                                                           ext), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableRemoveShort(BOARD_IEEE802154_DEV,
                                                           0x1003),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddShort(
                       BOARD_IEEE802154_DEV, 0x1000 + PENDING_TABLE_SZ),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddShort(
                       BOARD_IEEE802154_DEV, 0x1000 + PENDING_TABLE_SZ + 1),
                   WHAL_ENOTREADY);

    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableClear(BOARD_IEEE802154_DEV),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddShort(
                       BOARD_IEEE802154_DEV, 0x1000 + PENDING_TABLE_SZ + 1),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableClear(BOARD_IEEE802154_DEV),
                   WHAL_SUCCESS);
}

static void Test_Esp32c6_Ieee802154_PendingTableExtFull(void)
{
    uint8_t ext[WHAL_IEEE802154_EXT_ADDR_SZ];

    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableClear(BOARD_IEEE802154_DEV),
                   WHAL_SUCCESS);

    for (size_t i = 0; i < PENDING_TABLE_SZ; i++) {
        ExtAddr(ext, i);
        WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddExtended(
                           BOARD_IEEE802154_DEV, ext), WHAL_SUCCESS);
    }
    ExtAddr(ext, PENDING_TABLE_SZ);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddExtended(BOARD_IEEE802154_DEV,
                                                           ext),
                   WHAL_ENOTREADY);
    ExtAddr(ext, 3);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddExtended(BOARD_IEEE802154_DEV,
                                                           ext), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddShort(BOARD_IEEE802154_DEV,
                                                        0x2000), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableRemoveExtended(
                       BOARD_IEEE802154_DEV, ext), WHAL_SUCCESS);
    ExtAddr(ext, PENDING_TABLE_SZ);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddExtended(BOARD_IEEE802154_DEV,
                                                           ext), WHAL_SUCCESS);
    ExtAddr(ext, PENDING_TABLE_SZ + 1);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddExtended(BOARD_IEEE802154_DEV,
                                                           ext),
                   WHAL_ENOTREADY);

    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableClear(BOARD_IEEE802154_DEV),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableAddExtended(BOARD_IEEE802154_DEV,
                                                           ext), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Ieee802154_PendingTableClear(BOARD_IEEE802154_DEV),
                   WHAL_SUCCESS);
}

static void Test_Esp32c6_Ieee802154_AttrLimits(void)
{
    whal_Ieee802154_AttrVal channel;
    whal_Ieee802154_AttrVal ccaMode;
    whal_Ieee802154_AttrVal val;

    WHAL_ASSERT_EQ(whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_CHANNEL,
                                           &channel), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_CCA_MODE,
                                           &ccaMode), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(SetU8(WHAL_IEEE802154_ATTR_CHANNEL, CHANNEL_MIN - 1),
                   WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(SetU8(WHAL_IEEE802154_ATTR_CHANNEL, CHANNEL_MAX + 1),
                   WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(SetU8(WHAL_IEEE802154_ATTR_CHANNEL, CHANNEL_MIN),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(SetU8(WHAL_IEEE802154_ATTR_CHANNEL, CHANNEL_MAX),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(SetU8(WHAL_IEEE802154_ATTR_CCA_MODE,
                         WHAL_IEEE802154_CCA_MODE_ED - 1), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(SetU8(WHAL_IEEE802154_ATTR_CCA_MODE,
                         WHAL_IEEE802154_CCA_MODE_ED_OR_CARRIER + 1),
                   WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(SetU8(WHAL_IEEE802154_ATTR_CCA_MODE,
                         WHAL_IEEE802154_CCA_MODE_ED_OR_CARRIER),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(SetU8(WHAL_IEEE802154_ATTR_RX_SENSITIVITY + 1, 0),
                   WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_RX_SENSITIVITY
                                           + 1, &val), WHAL_ENOTSUP);

    WHAL_ASSERT_EQ(whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_RX_SENSITIVITY,
                                           &val), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val.i8, RX_SENSITIVITY_DBM);

    WHAL_ASSERT_EQ(whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_CHANNEL,
                                           &channel), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                           WHAL_IEEE802154_ATTR_CCA_MODE,
                                           &ccaMode), WHAL_SUCCESS);
}

static void Test_Esp32c6_Ieee802154_Unsupported(void)
{
    bool busy;

    WHAL_ASSERT_EQ(whal_Ieee802154_Cca(BOARD_IEEE802154_DEV, &busy),
                   WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Ieee802154_EnergyDetect(BOARD_IEEE802154_DEV, 0),
                   WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Ieee802154_EnergyDetect(BOARD_IEEE802154_DEV,
                                                SYMBOL_US - 1), WHAL_ENOTSUP);
}

static void Test_Esp32c6_Ieee802154_TxCfgLimits(void)
{
    const whal_Ieee802154_TxCfg valid = {
        .csmaCa = true,
        .maxCsmaBackoffs = 4,
        .minBe = 3,
        .maxBe = 5,
        .maxFrameRetries = 3,
    };
    whal_Ieee802154_TxCfg txCfg;
    uint8_t frame[] = { 0x41, 0x88, 0x00, 0xEF, 0xBE, 0xFF, 0xFF, 0x34, 0x12 };

    txCfg = valid;
    txCfg.maxCsmaBackoffs = 6;
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame,
                                            sizeof(frame), &txCfg),
                   WHAL_ENOTSUP);

    txCfg = valid;
    txCfg.maxBe = 2;
    txCfg.minBe = 2;
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame,
                                            sizeof(frame), &txCfg),
                   WHAL_ENOTSUP);

    txCfg = valid;
    txCfg.maxBe = 9;
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame,
                                            sizeof(frame), &txCfg),
                   WHAL_ENOTSUP);

    txCfg = valid;
    txCfg.minBe = 6;
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame,
                                            sizeof(frame), &txCfg),
                   WHAL_ENOTSUP);

    txCfg = valid;
    txCfg.maxFrameRetries = 8;
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame,
                                            sizeof(frame), &txCfg),
                   WHAL_ENOTSUP);

    txCfg = valid;
    txCfg.csmaCa = false;
    txCfg.maxFrameRetries = 8;
    WHAL_ASSERT_EQ(whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame,
                                            sizeof(frame), &txCfg),
                   WHAL_ENOTSUP);
}

void whal_Test_Ieee802154_Platform(void)
{
    WHAL_TEST_SUITE_START("ieee802154_platform");
    WHAL_TEST(Test_Esp32c6_Ieee802154_Caps);
    WHAL_TEST(Test_Esp32c6_Ieee802154_PendingTableShortFull);
    WHAL_TEST(Test_Esp32c6_Ieee802154_PendingTableExtFull);
    WHAL_TEST(Test_Esp32c6_Ieee802154_AttrLimits);
    WHAL_TEST(Test_Esp32c6_Ieee802154_Unsupported);
    WHAL_TEST(Test_Esp32c6_Ieee802154_TxCfgLimits);
    WHAL_TEST_SUITE_END();
}
