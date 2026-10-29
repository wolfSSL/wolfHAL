/* main.c
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

/*
 * Ping-pong between two boards over IEEE 802.15.4.
 *
 * Each board broadcasts a ping and waits for a ping from another board,
 * keeping its LED on while it waits. When one arrives the LED turns off, and
 * after a short pause the board sends its next ping. A board that hears
 * nothing for PING_TIMEOUT_MS sends again. Counters in g_pingStats can be
 * read over JTAG.
 */

#include <stdint.h>
#include <stdbool.h>
#include <wolfHAL/wolfHAL.h>
#include "wolfHAL_board.h"

#define PING_CHANNEL        26
#define PING_PAN_ID         0x1234
#define PING_BROADCAST      0xFFFF
#define PING_TX_POWER_DBM   0
#define PING_REPLY_DELAY_MS 500
#define PING_TIMEOUT_MS     2000

/* Data frame, PAN ID compression, short destination and source. */
#define PING_FCF            (0x0001 | 0x0040 | (2u << 10) | (2u << 14))
#define PING_HDR_SZ         9

#ifndef BOARD_LED_ON
#define BOARD_LED_ON        1
#endif

static const uint8_t g_pingPayload[] = { 'p', 'i', 'n', 'g' };

typedef struct {
    uint32_t sent;
    uint32_t received;
    uint32_t timeouts;
    uint32_t txFailed;
    uint16_t ownAddr;
    uint16_t peerAddr;
    int8_t lastRssi;
    uint8_t lastLqi;
} PingStats;

volatile PingStats g_pingStats;

static volatile bool g_gotPing;

static void Ping_Led(bool on)
{
    whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN,
                  on ? BOARD_LED_ON : !BOARD_LED_ON);
}

static void Ping_RxIndication(void *ctx, const whal_Ieee802154_RxInfo *rx)
{
    const uint8_t *f = rx->frame;
    (void)ctx;

    if (rx->len != PING_HDR_SZ + sizeof(g_pingPayload) ||
        (uint16_t)(f[0] | (f[1] << 8)) != PING_FCF) {
        return;
    }

    for (size_t i = 0; i < sizeof(g_pingPayload); i++) {
        if (f[PING_HDR_SZ + i] != g_pingPayload[i]) {
            return;
        }
    }

    g_pingStats.peerAddr = (uint16_t)(f[7] | (f[8] << 8));
    g_pingStats.lastRssi = rx->rssi;
    g_pingStats.lastLqi = rx->lqi;
    g_pingStats.received++;
    g_gotPing = true;
}

static void Ping_TxConfirm(void *ctx, whal_Ieee802154_TxStatus status,
                           const uint8_t *ack, size_t ackLen)
{
    (void)ctx;
    (void)ack;
    (void)ackLen;

    if (status != WHAL_IEEE802154_TX_SUCCESS) {
        g_pingStats.txFailed++;
    }
}

static const whal_Ieee802154_Callbacks g_pingCb = {
    .RxIndication = Ping_RxIndication,
    .TxConfirm = Ping_TxConfirm,
};

static whal_Error Ping_Setup(void)
{
    whal_Ieee802154_AttrVal val;
    whal_Error err;

    err = whal_Ieee802154_GetAttr(BOARD_IEEE802154_DEV,
                                  WHAL_IEEE802154_ATTR_EUI64, &val);
    if (err) {
        return err;
    }
    g_pingStats.ownAddr = (uint16_t)((val.bytes[6] << 8) | val.bytes[7]);
    g_pingStats.peerAddr = PING_BROADCAST;

    val.i8 = PING_TX_POWER_DBM;
    err = whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                  WHAL_IEEE802154_ATTR_TX_POWER, &val);
    if (err) {
        return err;
    }

    val.u8 = PING_CHANNEL;
    err = whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                  WHAL_IEEE802154_ATTR_CHANNEL, &val);
    if (err) {
        return err;
    }

    val.u16 = PING_PAN_ID;
    err = whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                  WHAL_IEEE802154_ATTR_PAN_ID, &val);
    if (err) {
        return err;
    }

    val.u16 = g_pingStats.ownAddr;
    err = whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                  WHAL_IEEE802154_ATTR_SHORT_ADDR, &val);
    if (err) {
        return err;
    }

    Board_Ieee802154SetCallbacks(&g_pingCb, NULL);

    val.u8 = 1;
    return whal_Ieee802154_SetAttr(BOARD_IEEE802154_DEV,
                                   WHAL_IEEE802154_ATTR_RX_ON_WHEN_IDLE, &val);
}

static void Ping_Send(uint8_t seq)
{
    static const whal_Ieee802154_TxCfg txCfg = {
        .csmaCa = true,
        .maxCsmaBackoffs = 4,
        .minBe = 3,
        .maxBe = 5,
        .maxFrameRetries = 0,
    };
    uint8_t frame[PING_HDR_SZ + sizeof(g_pingPayload)];
    uint16_t src = g_pingStats.ownAddr;

    frame[0] = (uint8_t)PING_FCF;
    frame[1] = (uint8_t)(PING_FCF >> 8);
    frame[2] = seq;
    frame[3] = (uint8_t)PING_PAN_ID;
    frame[4] = (uint8_t)(PING_PAN_ID >> 8);
    frame[5] = (uint8_t)PING_BROADCAST;
    frame[6] = (uint8_t)(PING_BROADCAST >> 8);
    frame[7] = (uint8_t)src;
    frame[8] = (uint8_t)(src >> 8);
    for (size_t i = 0; i < sizeof(g_pingPayload); i++) {
        frame[PING_HDR_SZ + i] = g_pingPayload[i];
    }

    if (whal_Ieee802154_Transmit(BOARD_IEEE802154_DEV, frame, sizeof(frame),
                                 &txCfg) != WHAL_SUCCESS) {
        g_pingStats.txFailed++;
        return;
    }
    g_pingStats.sent++;
}

static bool Ping_WaitForPing(void)
{
    for (uint32_t ms = 0; ms < PING_TIMEOUT_MS; ms++) {
        if (g_gotPing) {
            g_gotPing = false;
            return true;
        }
        Board_WaitMs(1);
    }
    return false;
}

void main(void)
{
    uint8_t seq = 0;

    if (Board_Init() != WHAL_SUCCESS)
        goto loop;

    if (Ping_Setup() != WHAL_SUCCESS)
        goto loop;

    while (1) {
        g_gotPing = false;
        Ping_Send(seq++);
        Ping_Led(true);

        if (Ping_WaitForPing()) {
            Ping_Led(false);
            Board_WaitMs(PING_REPLY_DELAY_MS);
        } else {
            g_pingStats.timeouts++;
        }
    }

loop:
    while (1);
}
