/* test_eth.c
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
#include <wolfHAL/eth_phy/eth_phy.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * Generic Ethernet tests.
 *
 * Tests MDIO access by reading standard IEEE 802.3 PHY ID registers
 * (registers 2 and 3). These are hardcoded in every PHY and readable
 * without a cable or link.
 */

/* IEEE 802.3 standard PHY registers */
#define PHY_REG_PHYIDR1 2
#define PHY_REG_PHYIDR2 3

static void Test_Eth_Api(void)
{
    uint8_t buf[64];
    size_t len = sizeof(buf);

    WHAL_ASSERT_EQ(whal_Eth_Send(BOARD_ETH_DEV, NULL, 64), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Eth_Recv(BOARD_ETH_DEV, NULL, &len), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Eth_Recv(BOARD_ETH_DEV, buf, NULL), WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Eth_MdioRead(BOARD_ETH_DEV, 0, 0, NULL), WHAL_EINVAL);
}

static void Test_Eth_MdioReadPhyId(void)
{
    uint16_t id1 = 0;
    uint16_t id2 = 0;

    WHAL_ASSERT_EQ(whal_Eth_MdioRead(BOARD_ETH_DEV, BOARD_ETH_PHY_ADDR,
                                      PHY_REG_PHYIDR1, &id1), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Eth_MdioRead(BOARD_ETH_DEV, BOARD_ETH_PHY_ADDR,
                                      PHY_REG_PHYIDR2, &id2), WHAL_SUCCESS);

    /* PHY ID should not be 0x0000 or 0xFFFF (no PHY / bus error) */
    WHAL_ASSERT_NEQ(id1, 0x0000);
    WHAL_ASSERT_NEQ(id1, 0xFFFF);
    WHAL_ASSERT_NEQ(id2, 0x0000);
    WHAL_ASSERT_NEQ(id2, 0xFFFF);

    /* Verify against board-defined expected PHY ID */
    WHAL_ASSERT_EQ(id1, BOARD_ETH_PHY_ID1);
    WHAL_ASSERT_EQ(id2, BOARD_ETH_PHY_ID2);
}


static void Test_Eth_PhyGetLinkState(void)
{
    uint8_t up = 0;
    uint8_t speed = 0;
    uint8_t duplex = 0;

    WHAL_ASSERT_EQ(whal_EthPhy_GetLinkState(BOARD_ETH_PHY_DEV, &up, &speed,
                                             &duplex), WHAL_SUCCESS);

    whal_Test_Printf("  link: %s, speed: %d, duplex: %s\n",
                     up ? "up" : "down", speed,
                     duplex ? "full" : "half");
}

void whal_Test_Eth(void)
{
    WHAL_TEST_SUITE_START("eth");
    WHAL_TEST(Test_Eth_Api);
    WHAL_TEST(Test_Eth_MdioReadPhyId);
    WHAL_TEST(Test_Eth_PhyGetLinkState);
    WHAL_TEST_SUITE_END();
}
