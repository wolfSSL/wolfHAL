/* lan8742a_eth_phy.h
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

#ifndef WHAL_LAN8742A_H
#define WHAL_LAN8742A_H

#include <wolfHAL/eth_phy/eth_phy.h>
#include <wolfHAL/timeout.h>

/*
 * @file lan8742a.h
 * @brief LAN8742A Ethernet PHY driver.
 *
 * The LAN8742A is a 10/100 Ethernet PHY from Microchip (formerly SMSC)
 * commonly found on STM32 Nucleo-144 boards. It connects to the MAC
 * via RMII and is configured over the MDIO bus.
 */

/*
 * @brief LAN8742A PHY configuration.
 */
typedef struct whal_Lan8742a_Cfg {
    whal_Timeout *timeout;
} whal_Lan8742a_Cfg;

/*
 * @brief Platform-owned LAN8742A PHY device singleton. Defined in the
 * driver TU from the WHAL_CFG_LAN8742A_DEV initializer in wolfHAL_board.h.
 */
extern const whal_EthPhy whal_Lan8742a_Dev;

#ifndef WHAL_CFG_LAN8742A_ETH_PHY_DIRECT_API_MAPPING
/*
 * @brief Driver instance for LAN8742A PHY.
 */
extern const whal_EthPhyDriver whal_Lan8742a_Driver;

/*
 * @brief Initialize the LAN8742A PHY.
 *
 * Resets the PHY, configures autonegotiation, and waits for link.
 *
 * @param phyDev PHY device instance.
 *
 * @retval WHAL_SUCCESS   PHY initialized.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ETIMEOUT  Link did not come up.
 */
whal_Error whal_Lan8742a_Init(whal_EthPhy *phyDev);

/*
 * @brief Deinitialize the LAN8742A PHY.
 *
 * Powers down the PHY.
 *
 * @param phyDev PHY device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Lan8742a_Deinit(whal_EthPhy *phyDev);

/*
 * @brief Get the LAN8742A link state.
 *
 * @param phyDev PHY device instance.
 * @param up     Output: 1 if link is up, 0 if down.
 * @param speed  Output: negotiated speed (10 or 100).
 *
 * @retval WHAL_SUCCESS Link state read.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Lan8742a_GetLinkState(whal_EthPhy *phyDev, uint8_t *up,
                                       uint8_t *speed, uint8_t *duplex);
#endif /* !WHAL_CFG_LAN8742A_ETH_PHY_DIRECT_API_MAPPING */

#endif /* WHAL_LAN8742A_H */
