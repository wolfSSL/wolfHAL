/* esp32c6_ieee802154.h
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

#ifndef WHAL_ESP32C6_IEEE802154_H
#define WHAL_ESP32C6_IEEE802154_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wolfHAL/ieee802154/ieee802154.h>

/*
 * @file esp32c6_ieee802154.h
 * @brief ESP32-C6 IEEE 802.15.4 radio driver.
 *
 * The 802.15.4 MAC block sends and receives frames, generates ACKs, filters
 * frames and runs single-shot CCA and energy detection. CSMA-CA backoff and
 * retransmission run in this driver on the MAC's timers. The RF front end is
 * brought up by Espressif's closed PHY and baseband libraries (libphy.a,
 * libbtbb.a, fetched by tools/esp32c6_radio/fetch_blobs.sh), which Init
 * calls.
 *
 * Before Init the board must enable the modem clocks (WHAL_ESP32C6_*_MODEM_CLK)
 * and keep the ROM's data at the top of SRAM (0x4087E610 and up) intact, as
 * the PHY library calls ROM routines that use it. Init runs a full RF
 * calibration, which takes a few hundred milliseconds.
 *
 * Events are reported from whal_Esp32c6_Ieee802154_IrqHandler, which the board
 * calls from the ZB_MAC interrupt (WHAL_ESP32C6_INTSRC_ZB_MAC).
 */

/*
 * @brief Radio device configuration.
 */
typedef struct {
    uint32_t cpuFreqMhz;  /* CPU clock, for the ROM delay routines */
    const uint8_t *eui64; /* Factory EUI-64, most significant byte first */
} whal_Esp32c6_Ieee802154_Cfg;

/*
 * @brief Single-instance device struct. Defined in the driver TU
 * from the WHAL_CFG_ESP32C6_IEEE802154_DEV initializer in wolfHAL_board.h.
 */
extern const whal_Ieee802154 whal_Esp32c6_Ieee802154_Dev;

/*
 * @brief Driver instance for the ESP32-C6 802.15.4 radio.
 */
extern const whal_Ieee802154Driver whal_Esp32c6_Ieee802154_Driver;

/*
 * @brief Calibrate the RF front end and initialize the MAC.
 *
 * Defaults: channel 11, 20 dBm, CCA by energy at -60 dBm, PAN ID and short
 * address 0xffff, receiver off, pending table disabled.
 *
 * @param radioDev Radio device instance.
 *
 * @retval WHAL_SUCCESS Radio initialized.
 */
whal_Error whal_Esp32c6_Ieee802154_Init(whal_Ieee802154 *radioDev);

/*
 * @brief Stop the MAC and power down the RF front end.
 *
 * @param radioDev Radio device instance.
 *
 * @retval WHAL_SUCCESS Radio stopped.
 */
whal_Error whal_Esp32c6_Ieee802154_Deinit(whal_Ieee802154 *radioDev);

/*
 * @brief Report the hardware capabilities.
 *
 * @param radioDev Radio device instance.
 * @param caps     Output for WHAL_IEEE802154_CAP_* flags.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_Ieee802154_GetCaps(whal_Ieee802154 *radioDev,
                                           uint32_t *caps);

/*
 * @brief Set a radio attribute. Takes effect at the next transmit or receive.
 *
 * @param radioDev Radio device instance.
 * @param attr     Attribute to set.
 * @param val      New value.
 *
 * @retval WHAL_SUCCESS Attribute set.
 * @retval WHAL_EINVAL  Read-only attribute.
 * @retval WHAL_ENOTSUP Unknown attribute, or channel outside 11-26, or CCA
 *                      mode outside 1-4.
 */
whal_Error whal_Esp32c6_Ieee802154_SetAttr(whal_Ieee802154 *radioDev,
                                           whal_Ieee802154_Attr attr,
                                           const whal_Ieee802154_AttrVal *val);

/*
 * @brief Get a radio attribute.
 *
 * @param radioDev Radio device instance.
 * @param attr     Attribute to read.
 * @param val      Output for the value.
 *
 * @retval WHAL_SUCCESS Attribute read.
 * @retval WHAL_ENOTSUP Unknown attribute, or EUI64 with no eui64 in the cfg.
 */
whal_Error whal_Esp32c6_Ieee802154_GetAttr(whal_Ieee802154 *radioDev,
                                           whal_Ieee802154_Attr attr,
                                           whal_Ieee802154_AttrVal *val);

/*
 * @brief Start transmitting a frame. The frame is copied, so the caller's
 * buffer may be reused once this returns.
 *
 * @param radioDev Radio device instance.
 * @param frame    MAC header and payload, no FCS.
 * @param len      Frame length.
 * @param txCfg    CSMA-CA and retry settings for this frame.
 *
 * @retval WHAL_SUCCESS   Transmit started.
 * @retval WHAL_ENOTREADY A transmit or energy detection is in progress.
 * @retval WHAL_ENOTSUP   txCfg values out of range.
 */
whal_Error whal_Esp32c6_Ieee802154_Transmit(whal_Ieee802154 *radioDev,
                                            const uint8_t *frame, size_t len,
                                            const whal_Ieee802154_TxCfg *txCfg);

/*
 * @brief Read the RSSI of the most recent receive.
 *
 * @param radioDev Radio device instance.
 * @param rssi     Output in dBm.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_Ieee802154_GetRssi(whal_Ieee802154 *radioDev,
                                           int8_t *rssi);

/*
 * @brief Not supported; CCA runs as part of Transmit.
 *
 * @param radioDev Radio device instance.
 * @param busy     Unused.
 *
 * @retval WHAL_ENOTSUP Always.
 */
whal_Error whal_Esp32c6_Ieee802154_Cca(whal_Ieee802154 *radioDev, bool *busy);

/*
 * @brief Start energy detection on the current channel. The peak level is
 * reported by EdConfirm.
 *
 * @param radioDev   Radio device instance.
 * @param durationUs Measurement duration, rounded down to 16 us symbols.
 *
 * @retval WHAL_SUCCESS   Detection started.
 * @retval WHAL_ENOTREADY A transmit or energy detection is in progress.
 * @retval WHAL_ENOTSUP   Duration shorter than one symbol or too long.
 */
whal_Error whal_Esp32c6_Ieee802154_EnergyDetect(whal_Ieee802154 *radioDev,
                                                uint32_t durationUs);

/*
 * @brief Enable or disable the frame pending table.
 *
 * @param radioDev Radio device instance.
 * @param enable   true to set Frame Pending only for listed sources.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_Ieee802154_PendingTableEnable(whal_Ieee802154 *radioDev,
                                                      bool enable);

/*
 * @brief Add a short address to the frame pending table.
 *
 * @param radioDev Radio device instance.
 * @param addr     Short address.
 *
 * @retval WHAL_SUCCESS   Address added or already present.
 * @retval WHAL_ENOTREADY Table full.
 */
whal_Error whal_Esp32c6_Ieee802154_PendingTableAddShort(whal_Ieee802154 *radioDev,
                                                        uint16_t addr);

/*
 * @brief Add an extended address to the frame pending table.
 *
 * @param radioDev Radio device instance.
 * @param addr     WHAL_IEEE802154_EXT_ADDR_SZ bytes, over-the-air order.
 *
 * @retval WHAL_SUCCESS   Address added or already present.
 * @retval WHAL_ENOTREADY Table full.
 */
whal_Error whal_Esp32c6_Ieee802154_PendingTableAddExtended(whal_Ieee802154 *radioDev,
                                                           const uint8_t *addr);

/*
 * @brief Remove a short address from the frame pending table.
 *
 * @param radioDev Radio device instance.
 * @param addr     Short address.
 *
 * @retval WHAL_SUCCESS Address removed or not present.
 */
whal_Error whal_Esp32c6_Ieee802154_PendingTableRemoveShort(whal_Ieee802154 *radioDev,
                                                           uint16_t addr);

/*
 * @brief Remove an extended address from the frame pending table.
 *
 * @param radioDev Radio device instance.
 * @param addr     WHAL_IEEE802154_EXT_ADDR_SZ bytes, over-the-air order.
 *
 * @retval WHAL_SUCCESS Address removed or not present.
 */
whal_Error whal_Esp32c6_Ieee802154_PendingTableRemoveExtended(whal_Ieee802154 *radioDev,
                                                              const uint8_t *addr);

/*
 * @brief Remove every entry from the frame pending table.
 *
 * @param radioDev Radio device instance.
 *
 * @retval WHAL_SUCCESS Always.
 */
whal_Error whal_Esp32c6_Ieee802154_PendingTableClear(whal_Ieee802154 *radioDev);

/*
 * @brief Handle the ZB_MAC interrupt and report events through cb.
 *
 * Call from the board's ISR for WHAL_ESP32C6_INTSRC_ZB_MAC. Callbacks run in
 * interrupt context.
 *
 * @param radioDev Radio device instance.
 * @param cb       Event callbacks; NULL or NULL members ignore those events.
 * @param ctx      Context passed to each callback.
 */
void whal_Esp32c6_Ieee802154_IrqHandler(whal_Ieee802154 *radioDev,
                                        const whal_Ieee802154_Callbacks *cb,
                                        void *ctx);

#endif /* WHAL_ESP32C6_IEEE802154_H */
