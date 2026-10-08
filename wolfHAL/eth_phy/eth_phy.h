/* eth_phy.h
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

#ifndef WHAL_ETH_PHY_H
#define WHAL_ETH_PHY_H

#include <wolfHAL/error.h>
#include <wolfHAL/eth/eth.h>
#include <stdint.h>

/*
 * @file eth_phy.h
 * @brief Generic Ethernet PHY abstraction and driver interface.
 *
 * The PHY driver handles link negotiation and status for an Ethernet
 * PHY device connected to a MAC via the MDIO bus. Different PHY chips
 * (e.g. LAN8742A, DP83848) have different init sequences but share
 * the same API.
 */

typedef struct whal_EthPhy whal_EthPhy;

/*
 * @brief Driver vtable for Ethernet PHY devices.
 */
typedef struct {
    /* Reset PHY and enable autonegotiation; does not wait for link. */
    whal_Error (*Init)(whal_EthPhy *phyDev);
    /* Power down the PHY. */
    whal_Error (*Deinit)(whal_EthPhy *phyDev);
    /* Read current link state: up/down, negotiated speed, and duplex. */
    whal_Error (*GetLinkState)(whal_EthPhy *phyDev, uint8_t *up,
                               uint8_t *speed, uint8_t *duplex);
} whal_EthPhyDriver;

/*
 * @brief Ethernet PHY device instance.
 *
 * The PHY is not memory-mapped. It accesses registers through the
 * parent MAC's MDIO bus via whal_Eth_MdioRead/MdioWrite.
 */
struct whal_EthPhy {
    whal_Eth *eth;                    /* MAC whose MDIO bus this PHY is on */
    uint8_t addr;                     /* PHY address on MDIO bus (0-31) */
    const whal_EthPhyDriver *driver;
    void *cfg;
};

/*
 * @brief Initialize an Ethernet PHY.
 *
 * Resets the PHY and enables autonegotiation. Does not wait for link; poll
 * whal_EthPhy_GetLinkState() for that.
 *
 * @param phyDev PHY device instance.
 *
 * @retval WHAL_SUCCESS   PHY reset and autonegotiation enabled.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ETIMEOUT  PHY reset did not complete within timeout.
 */
whal_Error whal_EthPhy_Init(whal_EthPhy *phyDev);
/*
 * @brief Deinitialize an Ethernet PHY.
 *
 * @param phyDev PHY device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_EthPhy_Deinit(whal_EthPhy *phyDev);
/*
 * @brief Get the current link state.
 *
 * @param phyDev PHY device instance.
 * @param up     Output: 1 if link is up, 0 if down.
 * @param speed  Output: negotiated speed (WHAL_ETH_SPEED_10/100).
 * @param duplex Output: duplex mode (WHAL_ETH_DUPLEX_HALF/FULL).
 *
 * @retval WHAL_SUCCESS   Link state read.
 * @retval WHAL_EINVAL    Invalid arguments.
 */
whal_Error whal_EthPhy_GetLinkState(whal_EthPhy *phyDev, uint8_t *up,
                                     uint8_t *speed, uint8_t *duplex);

#endif /* WHAL_ETH_PHY_H */
