/* ieee802154.h
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

#ifndef WHAL_IEEE802154_H
#define WHAL_IEEE802154_H

#include <wolfHAL/error.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * @file ieee802154.h
 * @brief Generic IEEE 802.15.4 radio abstraction and driver interface.
 *
 * Frames passed through this API are the MAC header and payload without the
 * FCS; the radio computes and checks the FCS. Transmit, receive and energy
 * detection complete asynchronously through the callbacks passed to the chip
 * driver's IrqHandler.
 */

typedef struct whal_Ieee802154 whal_Ieee802154;

/*
 * @brief Largest frame accepted by Transmit: 127 byte PSDU minus 2 byte FCS.
 */
#define WHAL_IEEE802154_FRAME_MAX 125

/*
 * @brief Length of an extended address or EUI-64.
 */
#define WHAL_IEEE802154_EXT_ADDR_SZ 8

/*
 * @brief Capability flags reported by GetCaps.
 */
#define WHAL_IEEE802154_CAP_AUTO_ACK      (1UL << 0) /* Sends ACKs */
#define WHAL_IEEE802154_CAP_ACK_WAIT      (1UL << 1) /* Waits for ACKs, reports NO_ACK */
#define WHAL_IEEE802154_CAP_CSMA_CA       (1UL << 2) /* Runs CSMA-CA */
#define WHAL_IEEE802154_CAP_RETRANSMIT    (1UL << 3) /* Retries unacknowledged frames */
#define WHAL_IEEE802154_CAP_FRAME_FILTER  (1UL << 4) /* Filters by PAN ID and address */
#define WHAL_IEEE802154_CAP_PROMISCUOUS   (1UL << 5) /* Can disable filtering */
#define WHAL_IEEE802154_CAP_PENDING_TABLE (1UL << 6) /* Sets Frame Pending in ACKs by table */
#define WHAL_IEEE802154_CAP_ENERGY_DETECT (1UL << 7) /* Supports EnergyDetect */
#define WHAL_IEEE802154_CAP_CCA           (1UL << 8) /* Supports standalone Cca */

/*
 * @brief Radio attributes for SetAttr and GetAttr.
 */
typedef enum {
    WHAL_IEEE802154_ATTR_CHANNEL,          /* u8: channel number */
    WHAL_IEEE802154_ATTR_TX_POWER,         /* i8: dBm */
    WHAL_IEEE802154_ATTR_CCA_MODE,         /* u8: WHAL_IEEE802154_CCA_MODE_* */
    WHAL_IEEE802154_ATTR_CCA_ED_THRESHOLD, /* i8: dBm */
    WHAL_IEEE802154_ATTR_PAN_ID,           /* u16 */
    WHAL_IEEE802154_ATTR_SHORT_ADDR,       /* u16 */
    WHAL_IEEE802154_ATTR_EXT_ADDR,         /* bytes: over-the-air order */
    WHAL_IEEE802154_ATTR_PROMISCUOUS,      /* u8: 0 or 1 */
    WHAL_IEEE802154_ATTR_RX_ON_WHEN_IDLE,  /* u8: 0 or 1 */
    WHAL_IEEE802154_ATTR_EUI64,            /* bytes: most significant first, read only */
    WHAL_IEEE802154_ATTR_RX_SENSITIVITY,   /* i8: dBm, read only */
} whal_Ieee802154_Attr;

/*
 * @brief Attribute value. The member used depends on the attribute.
 */
typedef union {
    uint8_t u8;
    int8_t i8;
    uint16_t u16;
    uint8_t bytes[WHAL_IEEE802154_EXT_ADDR_SZ];
} whal_Ieee802154_AttrVal;

/*
 * @brief Clear channel assessment modes.
 */
#define WHAL_IEEE802154_CCA_MODE_ED             1 /* Energy above threshold */
#define WHAL_IEEE802154_CCA_MODE_CARRIER        2 /* Carrier sense only */
#define WHAL_IEEE802154_CCA_MODE_ED_AND_CARRIER 3 /* Both */
#define WHAL_IEEE802154_CCA_MODE_ED_OR_CARRIER  4 /* Either */

/*
 * @brief Per-frame transmit configuration.
 */
typedef struct {
    bool csmaCa;             /* Run CSMA-CA before transmitting */
    uint8_t maxCsmaBackoffs; /* 0-5 */
    uint8_t minBe;           /* 0-maxBe */
    uint8_t maxBe;           /* 3-8 */
    uint8_t maxFrameRetries; /* 0-7, used when the frame requests an ACK */
} whal_Ieee802154_TxCfg;

/*
 * @brief Transmit completion status.
 */
typedef enum {
    WHAL_IEEE802154_TX_SUCCESS,
    WHAL_IEEE802154_TX_NO_ACK,
    WHAL_IEEE802154_TX_CHANNEL_ACCESS_FAILURE,
} whal_Ieee802154_TxStatus;

/*
 * @brief Received frame and its link measurements.
 */
typedef struct {
    const uint8_t *frame; /* MAC header and payload, no FCS */
    size_t len;
    int8_t rssi;          /* dBm */
    uint8_t lqi;          /* 0x00-0xff */
    bool ackFramePending; /* The ACK sent for this frame had Frame Pending set */
} whal_Ieee802154_RxInfo;

/*
 * @brief Event callbacks invoked by the chip driver's IrqHandler.
 *
 * Any member may be NULL to ignore that event.
 */
typedef struct {
    /* A frame passed filtering. rx is valid only during the call. */
    void (*RxIndication)(void *ctx, const whal_Ieee802154_RxInfo *rx);
    /* Transmit finished. ack is the received ACK frame or NULL. */
    void (*TxConfirm)(void *ctx, whal_Ieee802154_TxStatus status,
                      const uint8_t *ack, size_t ackLen);
    /* Energy detection finished. */
    void (*EdConfirm)(void *ctx, int8_t maxRssi);
} whal_Ieee802154_Callbacks;

/*
 * @brief Driver vtable for IEEE 802.15.4 radios.
 */
typedef struct {
    /* Power up and initialize the radio. */
    whal_Error (*Init)(whal_Ieee802154 *radioDev);
    /* Power down the radio. */
    whal_Error (*Deinit)(whal_Ieee802154 *radioDev);
    /* Report hardware capabilities. */
    whal_Error (*GetCaps)(whal_Ieee802154 *radioDev, uint32_t *caps);
    /* Set a radio attribute. */
    whal_Error (*SetAttr)(whal_Ieee802154 *radioDev, whal_Ieee802154_Attr attr,
                          const whal_Ieee802154_AttrVal *val);
    /* Get a radio attribute. */
    whal_Error (*GetAttr)(whal_Ieee802154 *radioDev, whal_Ieee802154_Attr attr,
                          whal_Ieee802154_AttrVal *val);
    /* Start transmitting a frame. */
    whal_Error (*Transmit)(whal_Ieee802154 *radioDev, const uint8_t *frame,
                           size_t len, const whal_Ieee802154_TxCfg *txCfg);
    /* Read the instantaneous RSSI. */
    whal_Error (*GetRssi)(whal_Ieee802154 *radioDev, int8_t *rssi);
    /* Perform one clear channel assessment. */
    whal_Error (*Cca)(whal_Ieee802154 *radioDev, bool *busy);
    /* Start energy detection. */
    whal_Error (*EnergyDetect)(whal_Ieee802154 *radioDev, uint32_t durationUs);
    /* Enable or disable the frame pending table. */
    whal_Error (*PendingTableEnable)(whal_Ieee802154 *radioDev, bool enable);
    /* Add a short address to the frame pending table. */
    whal_Error (*PendingTableAddShort)(whal_Ieee802154 *radioDev, uint16_t addr);
    /* Add an extended address to the frame pending table. */
    whal_Error (*PendingTableAddExtended)(whal_Ieee802154 *radioDev,
                                          const uint8_t *addr);
    /* Remove a short address from the frame pending table. */
    whal_Error (*PendingTableRemoveShort)(whal_Ieee802154 *radioDev,
                                          uint16_t addr);
    /* Remove an extended address from the frame pending table. */
    whal_Error (*PendingTableRemoveExtended)(whal_Ieee802154 *radioDev,
                                             const uint8_t *addr);
    /* Remove every entry from the frame pending table. */
    whal_Error (*PendingTableClear)(whal_Ieee802154 *radioDev);
} whal_Ieee802154Driver;

/*
 * @brief IEEE 802.15.4 radio instance with a base address, driver vtable, and
 * platform-specific cfg.
 */
struct whal_Ieee802154 {
    const size_t base;
    const whal_Ieee802154Driver *driver;
    void *cfg;
};

/*
 * @brief Initialize the radio. The receiver starts off.
 *
 * @param radioDev Radio device instance.
 *
 * @retval WHAL_SUCCESS Radio initialized.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Ieee802154_Init(whal_Ieee802154 *radioDev);

/*
 * @brief Power down the radio.
 *
 * @param radioDev Radio device instance.
 *
 * @retval WHAL_SUCCESS Radio deinitialized.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Ieee802154_Deinit(whal_Ieee802154 *radioDev);

/*
 * @brief Report which functions the radio performs in hardware.
 *
 * @param radioDev Radio device instance.
 * @param caps     Output for WHAL_IEEE802154_CAP_* flags.
 *
 * @retval WHAL_SUCCESS Capabilities reported.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Ieee802154_GetCaps(whal_Ieee802154 *radioDev, uint32_t *caps);

/*
 * @brief Set a radio attribute.
 *
 * @param radioDev Radio device instance.
 * @param attr     Attribute to set.
 * @param val      New value.
 *
 * @retval WHAL_SUCCESS Attribute set.
 * @retval WHAL_EINVAL  Invalid arguments, or a read-only attribute.
 * @retval WHAL_ENOTSUP Value or attribute not supported by the radio.
 */
whal_Error whal_Ieee802154_SetAttr(whal_Ieee802154 *radioDev,
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
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Attribute not supported by the radio.
 */
whal_Error whal_Ieee802154_GetAttr(whal_Ieee802154 *radioDev,
                                   whal_Ieee802154_Attr attr,
                                   whal_Ieee802154_AttrVal *val);

/*
 * @brief Start transmitting a frame. Completion is reported by TxConfirm.
 *
 * The frame's Acknowledgment Request field decides whether the radio waits
 * for an ACK. The radio returns to the state set by RX_ON_WHEN_IDLE after
 * the transmit completes.
 *
 * @param radioDev Radio device instance.
 * @param frame    MAC header and payload, no FCS.
 * @param len      Frame length, at most WHAL_IEEE802154_FRAME_MAX.
 * @param txCfg    CSMA-CA and retry settings for this frame.
 *
 * @retval WHAL_SUCCESS   Transmit started.
 * @retval WHAL_EINVAL    Invalid arguments or frame too long.
 * @retval WHAL_ENOTREADY A transmit is already in progress.
 * @retval WHAL_ENOTSUP   txCfg requests CSMA-CA or retries the radio lacks.
 */
whal_Error whal_Ieee802154_Transmit(whal_Ieee802154 *radioDev,
                                    const uint8_t *frame, size_t len,
                                    const whal_Ieee802154_TxCfg *txCfg);

/*
 * @brief Read the instantaneous received signal strength on the current
 * channel.
 *
 * @param radioDev Radio device instance.
 * @param rssi     Output in dBm.
 *
 * @retval WHAL_SUCCESS RSSI read.
 * @retval WHAL_EINVAL  Invalid arguments.
 */
whal_Error whal_Ieee802154_GetRssi(whal_Ieee802154 *radioDev, int8_t *rssi);

/*
 * @brief Perform one clear channel assessment in the configured CCA mode.
 *
 * @param radioDev Radio device instance.
 * @param busy     Output, true if the channel is busy.
 *
 * @retval WHAL_SUCCESS Assessment completed.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Radio has no standalone CCA.
 */
whal_Error whal_Ieee802154_Cca(whal_Ieee802154 *radioDev, bool *busy);

/*
 * @brief Start energy detection on the current channel. The peak level is
 * reported by EdConfirm.
 *
 * @param radioDev   Radio device instance.
 * @param durationUs Measurement duration in microseconds.
 *
 * @retval WHAL_SUCCESS   Detection started.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ENOTREADY Radio busy.
 * @retval WHAL_ENOTSUP   Radio has no energy detection.
 */
whal_Error whal_Ieee802154_EnergyDetect(whal_Ieee802154 *radioDev,
                                        uint32_t durationUs);

/*
 * @brief Enable or disable setting Frame Pending in ACKs from the pending
 * table. When disabled, every ACK to a Data Request has Frame Pending set.
 *
 * @param radioDev Radio device instance.
 * @param enable   true to use the table.
 *
 * @retval WHAL_SUCCESS Setting applied.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Radio has no pending table.
 */
whal_Error whal_Ieee802154_PendingTableEnable(whal_Ieee802154 *radioDev,
                                              bool enable);

/*
 * @brief Add a short address to the pending table.
 *
 * @param radioDev Radio device instance.
 * @param addr     Short address.
 *
 * @retval WHAL_SUCCESS   Address added.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ENOTREADY Table full.
 * @retval WHAL_ENOTSUP   Radio has no pending table.
 */
whal_Error whal_Ieee802154_PendingTableAddShort(whal_Ieee802154 *radioDev,
                                                uint16_t addr);

/*
 * @brief Add an extended address to the pending table.
 *
 * @param radioDev Radio device instance.
 * @param addr     WHAL_IEEE802154_EXT_ADDR_SZ bytes, over-the-air order.
 *
 * @retval WHAL_SUCCESS   Address added.
 * @retval WHAL_EINVAL    Invalid arguments.
 * @retval WHAL_ENOTREADY Table full.
 * @retval WHAL_ENOTSUP   Radio has no pending table.
 */
whal_Error whal_Ieee802154_PendingTableAddExtended(whal_Ieee802154 *radioDev,
                                                   const uint8_t *addr);

/*
 * @brief Remove a short address from the pending table.
 *
 * @param radioDev Radio device instance.
 * @param addr     Short address.
 *
 * @retval WHAL_SUCCESS Address removed or not present.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Radio has no pending table.
 */
whal_Error whal_Ieee802154_PendingTableRemoveShort(whal_Ieee802154 *radioDev,
                                                   uint16_t addr);

/*
 * @brief Remove an extended address from the pending table.
 *
 * @param radioDev Radio device instance.
 * @param addr     WHAL_IEEE802154_EXT_ADDR_SZ bytes, over-the-air order.
 *
 * @retval WHAL_SUCCESS Address removed or not present.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Radio has no pending table.
 */
whal_Error whal_Ieee802154_PendingTableRemoveExtended(whal_Ieee802154 *radioDev,
                                                      const uint8_t *addr);

/*
 * @brief Remove every entry from the pending table.
 *
 * @param radioDev Radio device instance.
 *
 * @retval WHAL_SUCCESS Table cleared.
 * @retval WHAL_EINVAL  Invalid arguments.
 * @retval WHAL_ENOTSUP Radio has no pending table.
 */
whal_Error whal_Ieee802154_PendingTableClear(whal_Ieee802154 *radioDev);

#endif /* WHAL_IEEE802154_H */
