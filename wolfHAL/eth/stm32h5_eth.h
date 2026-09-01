/* stm32h5_eth.h
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

#ifndef WHAL_STM32H5_ETH_H
#define WHAL_STM32H5_ETH_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/eth/eth.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32h5_eth.h
 * @brief STM32H5 Ethernet MAC driver configuration.
 *
 * The STM32H5 Ethernet peripheral is a Synopsys DWC Ethernet MAC with
 * integrated DMA controller. It supports 10/100 Mbps via MII or RMII
 * interface, hardware checksum offload, and IEEE 1588 timestamping.
 *
 * The driver uses descriptor rings in RAM for TX and RX DMA transfers.
 * Descriptor and buffer memory must be provided by the board configuration.
 */

/* TX DMA descriptor (4 x 32-bit words, 16 bytes) */
typedef struct {
    volatile uint32_t des[4];
} whal_Stm32h5_Eth_TxDesc;

/* RX DMA descriptor (4 x 32-bit words, 16 bytes) */
typedef struct {
    volatile uint32_t des[4];
} whal_Stm32h5_Eth_RxDesc;

/*
 * @brief STM32H5 Ethernet MAC configuration.
 */
typedef struct whal_Stm32h5_Eth_Cfg {
    whal_Stm32h5_Eth_TxDesc *txDescs;     /* TX descriptor ring (pre-allocated) */
    uint8_t *txBufs;                      /* TX frame buffers (pre-allocated) */
    size_t txDescCount;                   /* Number of TX descriptors */
    size_t txBufSize;                     /* Size of each TX buffer in bytes */
    whal_Stm32h5_Eth_RxDesc *rxDescs;     /* RX descriptor ring (pre-allocated) */
    uint8_t *rxBufs;                      /* RX frame buffers (pre-allocated) */
    size_t rxDescCount;                   /* Number of RX descriptors */
    size_t rxBufSize;                     /* Size of each RX buffer in bytes */
    /* MDIO clock range select (CR field in MACMDIOAR). Picks the AHB->MDC
     * divider for the board's HCLK; MDC must stay <= 2.5 MHz (IEEE 802.3).
     *   0: 60-100 MHz (/42)   1: 100-150 MHz (/62)
     *   2: 20-35  MHz (/16)   3: 35-60   MHz (/26)
     *   4: 150-250MHz (/102)  5: 250-300 MHz (/124) */
    uint8_t mdioCr;
    whal_Timeout *timeout;
} whal_Stm32h5_Eth_Cfg;

/*
 * @brief Platform-owned Ethernet device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32H5_ETH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Eth whal_Stm32h5_Eth_Dev;

#ifndef WHAL_CFG_STM32H5_ETH_DIRECT_API_MAPPING
/*
 * @brief Driver instance for STM32H5 Ethernet MAC.
 */
extern const whal_EthDriver whal_Stm32h5_Eth_Driver;

/*
 * @brief Initialize the STM32H5 Ethernet MAC.
 *
 * Configures the MAC, MTL, and DMA. Sets up descriptor rings and
 * MAC address. Does not start TX/RX.
 *
 * @param ethDev Ethernet device instance.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Eth_Init(whal_Eth *ethDev);

/*
 * @brief Deinitialize the STM32H5 Ethernet MAC.
 *
 * @param ethDev Ethernet device instance.
 *
 * @retval WHAL_SUCCESS Deinit completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Eth_Deinit(whal_Eth *ethDev);

/*
 * @brief Start the Ethernet MAC TX/RX and DMA engines.
 *
 * Configures MAC speed and duplex, then enables TX/RX and starts DMA.
 *
 * @param ethDev Ethernet device instance.
 * @param speed  Link speed: WHAL_ETH_SPEED_10 or WHAL_ETH_SPEED_100.
 * @param duplex Duplex mode: WHAL_ETH_DUPLEX_HALF or WHAL_ETH_DUPLEX_FULL.
 *
 * @retval WHAL_SUCCESS MAC started.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Eth_Start(whal_Eth *ethDev, uint8_t speed,
                                  uint8_t duplex);

/*
 * @brief Stop the Ethernet MAC TX/RX and DMA engines.
 *
 * @param ethDev Ethernet device instance.
 *
 * @retval WHAL_SUCCESS MAC stopped.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Eth_Stop(whal_Eth *ethDev);

/*
 * @brief Transmit an Ethernet frame.
 *
 * Copies frame data into the next available TX descriptor buffer,
 * sets the OWN bit, and advances the DMA tail pointer.
 *
 * @param ethDev Ethernet device instance.
 * @param frame  Frame data to transmit.
 * @param len    Length of the frame in bytes.
 *
 * @retval WHAL_SUCCESS   Frame queued for transmission.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ENOTREADY No TX descriptor available.
 */
whal_Error whal_Stm32h5_Eth_Send(whal_Eth *ethDev, const void *frame,
                                 size_t len);

/*
 * @brief Receive an Ethernet frame.
 *
 * Checks if the DMA has completed an RX descriptor. If so, copies
 * the frame data into the caller's buffer and returns the descriptor
 * to DMA.
 *
 * @param ethDev Ethernet device instance.
 * @param frame  Buffer to receive frame data into.
 * @param len    On entry, buffer size. On exit, received frame length.
 *
 * @retval WHAL_SUCCESS   Frame received.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ENOTREADY No frame available.
 */
whal_Error whal_Stm32h5_Eth_Recv(whal_Eth *ethDev, void *frame,
                                 size_t *len);

/*
 * @brief Read a PHY register via MDIO.
 *
 * @param ethDev  Ethernet device instance.
 * @param phyAddr PHY address on the MDIO bus (0-31).
 * @param reg     PHY register address (0-31).
 * @param val     Output for the 16-bit register value.
 *
 * @retval WHAL_SUCCESS   Register read completed.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ETIMEOUT  MDIO busy timeout.
 */
whal_Error whal_Stm32h5_Eth_MdioRead(whal_Eth *ethDev, uint8_t phyAddr,
                                      uint8_t reg, uint16_t *val);

/*
 * @brief Write a PHY register via MDIO.
 *
 * @param ethDev  Ethernet device instance.
 * @param phyAddr PHY address on the MDIO bus (0-31).
 * @param reg     PHY register address (0-31).
 * @param val     16-bit value to write.
 *
 * @retval WHAL_SUCCESS   Register write completed.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ETIMEOUT  MDIO busy timeout.
 */
whal_Error whal_Stm32h5_Eth_MdioWrite(whal_Eth *ethDev, uint8_t phyAddr,
                                       uint8_t reg, uint16_t val);
#endif /* !WHAL_CFG_STM32H5_ETH_DIRECT_API_MAPPING */

/*
 * @brief Enable or disable MAC-internal loopback.
 *
 * When enabled, the TX data path feeds directly into the RX data path
 * inside the MAC. No PHY, cable, or link partner is needed.
 *
 * @param ethDev Ethernet device instance.
 * @param enable 1 to enable loopback, 0 to disable.
 *
 * @retval WHAL_SUCCESS Loopback state changed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32h5_Eth_Ext_EnableLoopback(whal_Eth *ethDev,
                                                uint8_t enable);

#endif /* WHAL_STM32H5_ETH_H */
