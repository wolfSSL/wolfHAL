/* stm32n6_eth.h
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

#ifndef WHAL_STM32N6_ETH_H
#define WHAL_STM32N6_ETH_H

/**
 * @file stm32n6_eth.h
 * @brief STM32N6 Ethernet MAC driver configuration.
 *
 * The STM32N6 Ethernet peripheral uses the same Synopsys DWC EQOS GMAC IP
 * as the STM32H5, with these differences:
 * - AXI 64-bit bus (vs AHB 32-bit) with ACE coherency registers
 * - 2 DMA channels with 0x80 stride (this driver uses channel 0 only)
 * - Descriptor alignment: bits 2:0 reserved (8-byte aligned)
 * - New AXI registers at DMA offsets 0x1020-0x1028
 *
 * ETH1 base address: 0x48036000
 *
 * The driver uses descriptor rings in RAM for TX and RX DMA transfers.
 * Descriptor and buffer memory must be provided by the board configuration.
 * Descriptors must be 8-byte aligned.
 */

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/eth/eth.h>
#include <wolfHAL/timeout.h>

/** TX DMA descriptor (4 x 32-bit words, 16 bytes, must be 8-byte aligned) */
typedef struct {
    volatile uint32_t des[4];
} whal_Stm32n6_Eth_TxDesc;

/** RX DMA descriptor (4 x 32-bit words, 16 bytes, must be 8-byte aligned) */
typedef struct {
    volatile uint32_t des[4];
} whal_Stm32n6_Eth_RxDesc;

/**
 * @brief STM32N6 Ethernet MAC configuration.
 */
typedef struct whal_Stm32n6_Eth_Cfg {
    whal_Stm32n6_Eth_TxDesc *txDescs;     /**< TX descriptor ring (8-byte aligned) */
    uint8_t *txBufs;                      /**< TX frame buffers */
    size_t txDescCount;                   /**< Number of TX descriptors */
    size_t txBufSize;                     /**< Size of each TX buffer in bytes */
    whal_Stm32n6_Eth_RxDesc *rxDescs;     /**< RX descriptor ring (8-byte aligned) */
    uint8_t *rxBufs;                      /**< RX frame buffers */
    size_t rxDescCount;                   /**< Number of RX descriptors */
    size_t rxBufSize;                     /**< Size of each RX buffer in bytes */
    /* MDIO clock range select (CR field in MACMDIOAR). Picks the AXI->MDC
     * divider for the board's HCLK; MDC must stay <= 2.5 MHz (IEEE 802.3).
     *   0: 60-100 MHz (/42)   1: 100-150 MHz (/62)
     *   2: 20-35  MHz (/16)   3: 35-60   MHz (/26)
     *   4: 150-250MHz (/102)  5: 250-300 MHz (/124) */
    uint8_t mdioCr;
    whal_Timeout *timeout;
} whal_Stm32n6_Eth_Cfg;

/*
 * @brief Platform-owned Ethernet device singleton. Defined in the driver TU
 * from the WHAL_CFG_STM32N6_ETH_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Eth whal_Stm32n6_Eth_Dev;

#ifndef WHAL_CFG_STM32N6_ETH_DIRECT_API_MAPPING
/**
 * @brief Driver instance for STM32N6 Ethernet MAC.
 */
extern const whal_EthDriver whal_Stm32n6_Eth_Driver;

/**
 * @brief Initialize the STM32N6 Ethernet MAC.
 *
 * Resets the DMA, programs descriptor base addresses from cfg, configures
 * MTL/MAC operating modes, and leaves the MAC ready for `Start`.
 *
 * @param ethDev Ethernet device instance.
 *
 * @retval WHAL_SUCCESS  MAC initialized.
 * @retval WHAL_EINVAL   Null pointer or missing cfg.
 * @retval WHAL_ETIMEOUT DMA software-reset did not clear within the timeout.
 */
whal_Error whal_Stm32n6_Eth_Init(whal_Eth *ethDev);

/**
 * @brief Deinitialize the STM32N6 Ethernet MAC and stop DMA.
 *
 * @param ethDev Ethernet device instance.
 *
 * @retval WHAL_SUCCESS  MAC deinitialized.
 * @retval WHAL_EINVAL   Null pointer.
 */
whal_Error whal_Stm32n6_Eth_Deinit(whal_Eth *ethDev);

/**
 * @brief Start the MAC at the given speed/duplex (after PHY autonegotiation).
 *
 * @param ethDev Ethernet device instance.
 * @param speed  Link speed (10 or 100 Mbps; gigabit unsupported on this MAC).
 * @param duplex 0 = half duplex, 1 = full duplex.
 *
 * @retval WHAL_SUCCESS  TX/RX enabled.
 * @retval WHAL_EINVAL   Null pointer or unsupported speed.
 */
whal_Error whal_Stm32n6_Eth_Start(whal_Eth *ethDev, uint8_t speed,
                                  uint8_t duplex);

/**
 * @brief Stop the MAC (disables TX/RX).
 *
 * @param ethDev Ethernet device instance.
 *
 * @retval WHAL_SUCCESS  TX/RX disabled.
 * @retval WHAL_EINVAL   Null pointer.
 */
whal_Error whal_Stm32n6_Eth_Stop(whal_Eth *ethDev);

/**
 * @brief Send a single Ethernet frame via DMA.
 *
 * @param ethDev Ethernet device instance.
 * @param frame  Pointer to the frame to send (DMA-accessible memory).
 * @param len    Frame length in bytes.
 *
 * @retval WHAL_SUCCESS  Frame queued for transmission.
 * @retval WHAL_EINVAL   Null pointer or length exceeds buffer size.
 * @retval WHAL_ENOTREADY No free TX descriptor.
 */
whal_Error whal_Stm32n6_Eth_Send(whal_Eth *ethDev, const void *frame,
                                 size_t len);

/**
 * @brief Receive an Ethernet frame from the RX descriptor ring.
 *
 * @param ethDev Ethernet device instance.
 * @param frame  Buffer to receive into.
 * @param len    In: capacity of `frame`. Out: actual frame length.
 *
 * @retval WHAL_SUCCESS   Frame received.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ENOTREADY No frame available.
 */
whal_Error whal_Stm32n6_Eth_Recv(whal_Eth *ethDev, void *frame,
                                 size_t *len);

/**
 * @brief MDIO read of a PHY register (clause-22).
 *
 * @param ethDev  Ethernet device instance.
 * @param phyAddr 5-bit PHY address (0–31).
 * @param reg     5-bit register address.
 * @param val     Out: register value read.
 *
 * @retval WHAL_SUCCESS   Read completed.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ETIMEOUT  MDIO transaction did not complete within the timeout.
 */
whal_Error whal_Stm32n6_Eth_MdioRead(whal_Eth *ethDev, uint8_t phyAddr,
                                      uint8_t reg, uint16_t *val);

/**
 * @brief MDIO write of a PHY register (clause-22).
 *
 * @param ethDev  Ethernet device instance.
 * @param phyAddr 5-bit PHY address (0–31).
 * @param reg     5-bit register address.
 * @param val     Value to write.
 *
 * @retval WHAL_SUCCESS   Write completed.
 * @retval WHAL_EINVAL    Null pointer.
 * @retval WHAL_ETIMEOUT  MDIO transaction did not complete within the timeout.
 */
whal_Error whal_Stm32n6_Eth_MdioWrite(whal_Eth *ethDev, uint8_t phyAddr,
                                       uint8_t reg, uint16_t val);
#endif /* !WHAL_CFG_STM32N6_ETH_DIRECT_API_MAPPING */

/**
 * @brief Enable or disable MAC-internal loopback.
 *
 * @param ethDev Ethernet device instance.
 * @param enable 1 to enable loopback, 0 to disable.
 * @retval WHAL_SUCCESS Loopback state changed.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Stm32n6_Eth_Ext_EnableLoopback(whal_Eth *ethDev,
                                                uint8_t enable);

#endif /* WHAL_STM32N6_ETH_H */
