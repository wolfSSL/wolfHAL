/* esp32c6_ieee802154.c
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

#include "wolfHAL_board.h"  /* provides WHAL_CFG_ESP32C6_IEEE802154_DEV initializer */
#include <wolfHAL/ieee802154/esp32c6_ieee802154.h>
#include <wolfHAL/ieee802154/ieee802154.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

const whal_Ieee802154 whal_Esp32c6_Ieee802154_Dev = WHAL_CFG_ESP32C6_IEEE802154_DEV;

/* 802.15.4 MAC registers. The block is not in the TRM; offsets follow
 * Espressif's register description in ESP-IDF. */

#define ZB_COMMAND_REG                  0x0000
#define ZB_CMD_TX_START                 0x41
#define ZB_CMD_RX_START                 0x42
#define ZB_CMD_CCA_TX_START             0x43
#define ZB_CMD_ED_START                 0x44
#define ZB_CMD_STOP                     0x45
#define ZB_CMD_TIMER0_START             0x4C
#define ZB_CMD_TIMER0_STOP              0x4D
#define ZB_CMD_TIMER1_START             0x4E
#define ZB_CMD_TIMER1_STOP              0x4F

#define ZB_CTRL_CFG_REG                 0x0004
#define ZB_AUTO_ACK_TX_Msk              (1UL << 0)
#define ZB_AUTO_ACK_RX_Msk              (1UL << 3)
#define ZB_COORDINATOR_Msk              (1UL << 6)
#define ZB_PROMISCUOUS_Msk              (1UL << 7)
#define ZB_PENDING_ENHANCE_Msk          (1UL << 12)
#define ZB_MULTIPAN_Pos                 28
#define ZB_MULTIPAN_Msk                 (WHAL_BITMASK(4) << ZB_MULTIPAN_Pos)

#define ZB_INF0_SHORT_ADDR_REG          0x0008
#define ZB_INF0_PAN_ID_REG              0x000C
#define ZB_INF0_EXT_ADDR0_REG           0x0010
#define ZB_INF0_EXT_ADDR1_REG           0x0014

#define ZB_CHANNEL_REG                  0x0048
#define ZB_HOP_Pos                      0
#define ZB_HOP_Msk                      (WHAL_BITMASK(7) << ZB_HOP_Pos)

#define ZB_TX_POWER_REG                 0x004C
#define ZB_TX_POWER_Pos                 0
#define ZB_TX_POWER_Msk                 (WHAL_BITMASK(5) << ZB_TX_POWER_Pos)

#define ZB_ED_DURATION_REG              0x0050
#define ZB_ED_DURATION_Pos              0
#define ZB_ED_DURATION_Msk              (WHAL_BITMASK(24) << ZB_ED_DURATION_Pos)

#define ZB_ED_CFG_REG                   0x0054
#define ZB_CCA_THRESHOLD_Pos            0
#define ZB_CCA_THRESHOLD_Msk            (WHAL_BITMASK(8) << ZB_CCA_THRESHOLD_Pos)
#define ZB_ED_PWR_SEL_AVG_Msk           (1UL << 13)
#define ZB_CCA_MODE_Pos                 14
#define ZB_CCA_MODE_Msk                 (WHAL_BITMASK(2) << ZB_CCA_MODE_Pos)
#define ZB_ED_RSS_Pos                   16
#define ZB_ED_RSS_Msk                   (WHAL_BITMASK(8) << ZB_ED_RSS_Pos)

/* Hardware CCA mode encoding. */
#define ZB_CCA_CARRIER                  0
#define ZB_CCA_ED                       1
#define ZB_CCA_CARRIER_OR_ED            2
#define ZB_CCA_CARRIER_AND_ED           3

#define ZB_EVENT_EN_REG                 0x0060
#define ZB_EVENT_STATUS_REG             0x0064
#define ZB_EVT_TX_DONE                  (1UL << 0)
#define ZB_EVT_RX_DONE                  (1UL << 1)
#define ZB_EVT_ACK_TX_DONE              (1UL << 2)
#define ZB_EVT_ACK_RX_DONE              (1UL << 3)
#define ZB_EVT_RX_ABORT                 (1UL << 4)
#define ZB_EVT_TX_ABORT                 (1UL << 5)
#define ZB_EVT_ED_DONE                  (1UL << 6)
#define ZB_EVT_TIMER0                   (1UL << 8)
#define ZB_EVT_TIMER1                   (1UL << 9)
#define ZB_EVT_ALL                      WHAL_BITMASK(13)

#define ZB_RX_ABORT_EN_REG              0x0068
#define ZB_ACK_PENDING_REG              0x006C
#define ZB_ACK_PENDING_Msk              (1UL << 0)

#define ZB_COEX_PTI_REG                 0x0070
#define ZB_COEX_PTI_DISABLED            0x33

#define ZB_TX_ABORT_EN_REG              0x0078

/* Abort reasons are reported as a number; the enable registers take bit
 * (reason - 1). */
#define ZB_ABORT_BIT(reason)            (1UL << ((reason) - 1))

#define ZB_RX_STATUS_REG                0x0080
#define ZB_RX_ABORT_Pos                 4
#define ZB_RX_ABORT_Msk                 (WHAL_BITMASK(5) << ZB_RX_ABORT_Pos)
#define ZB_RX_STATE_Pos                 16
#define ZB_RX_STATE_Msk                 (WHAL_BITMASK(3) << ZB_RX_STATE_Pos)
#define ZB_RX_STATE_SFD                 1

#define ZB_RX_ABORT_TX_ACK_TIMEOUT      16
#define ZB_RX_ABORT_TX_ACK_COEX_BREAK   18

#define ZB_TX_STATUS_REG                0x0084
#define ZB_TX_ABORT_Pos                 4
#define ZB_TX_ABORT_Msk                 (WHAL_BITMASK(5) << ZB_TX_ABORT_Pos)

#define ZB_TX_ABORT_RX_ACK_SFD_TIMEOUT  2
#define ZB_TX_ABORT_RX_ACK_RESTART      9
#define ZB_TX_ABORT_RX_ACK_TIMEOUT      16
#define ZB_TX_ABORT_TX_STOP             17
#define ZB_TX_ABORT_TX_COEX_BREAK       18
#define ZB_TX_ABORT_TX_SECURITY_ERROR   19
#define ZB_TX_ABORT_CCA_FAILED          24
#define ZB_TX_ABORT_CCA_BUSY            25

#define ZB_TIME0_THRESHOLD_REG          0x00A8
#define ZB_TIME1_THRESHOLD_REG          0x00B0

#define ZB_TXDMA_ADDR_REG               0x00D0
#define ZB_RXDMA_ADDR_REG               0x00E0

#define ZB_SEC_CTRL_REG                 0x0128
#define ZB_SEC_EN_Msk                   (1UL << 0)

/* 2450 MHz band: channel k is at 2405 + 5(k - 11) MHz; the HOP field counts
 * MHz above 2400. */
#define ZB_CHANNEL_MIN                  11
#define ZB_CHANNEL_MAX                  26
#define ZB_CHANNEL_TO_HOP(ch)           (((ch) - ZB_CHANNEL_MIN) * 5 + 3)

/* TX power index: 3 (-15 dBm) to 15 (+20 dBm) in 3 dB steps. */
#define ZB_TX_POWER_DBM_MIN             (-15)
#define ZB_TX_POWER_DBM_MAX             20
#define ZB_TX_POWER_IDX_MIN             3
#define ZB_TX_POWER_IDX_MAX             15

#define ZB_RX_SENSITIVITY_DBM           (-104)
#define ZB_SYMBOL_US                    16
#define ZB_CCA_SYMBOLS                  8
#define ZB_UNIT_BACKOFF_US              (20 * ZB_SYMBOL_US)
#define ZB_ACK_WAIT_LIMIT_US            200000
#define ZB_PENDING_TABLE_SZ             16
#define ZB_BB_TX_ON_DELAY               50

/* Frame control field. */
#define FCF_FRAME_TYPE_Msk              0x0007
#define FCF_FRAME_TYPE_ACK              2
#define FCF_ACK_REQUEST_Msk             0x0020
#define FCF_PAN_ID_COMP_Msk             0x0040
#define FCF_DST_MODE_Pos                10
#define FCF_VERSION_Pos                 12
#define FCF_SRC_MODE_Pos                14
#define FCF_ADDR_MODE_NONE              0
#define FCF_ADDR_MODE_SHORT             2
#define FCF_VERSION_2006                1

/* Calibration table the PHY library fills during a full calibration. */
#define PHY_CAL_DATA_SZ                 1904
#define PHY_RF_CAL_FULL                 2

/* Espressif PHY and baseband library entry points (libphy.a, libbtbb.a) and
 * ROM routines (esp32c6.rom.ld). */
extern void phy_init_param_set(uint8_t param);
extern void phy_bbpll_en_usb(bool en);
extern int register_chipv7_phy(const void *initData, void *calData,
                               int calMode);
extern void phy_close_rf(void);
extern void phy_xpd_tsens(void);
extern void bt_bb_v2_init_cmplx(int printVersion);
extern void bt_bb_set_zb_tx_on_delay(uint16_t delay);
extern uint32_t bt_bb_get_cur_rx_info(void);
extern void ieee802154_txon_delay_set(void);
extern void ets_update_cpu_frequency(uint32_t ticksPerUs);

typedef enum {
    ZB_STATE_IDLE,
    ZB_STATE_RX,
    ZB_STATE_TX_BACKOFF,
    ZB_STATE_TX_CCA,
    ZB_STATE_TX,
    ZB_STATE_RX_ACK,
    ZB_STATE_TX_ACK,
    ZB_STATE_ED,
} ZbState;

typedef struct {
    uint8_t channel;
    int8_t txPower;
    uint8_t ccaMode;
    int8_t ccaThreshold;
    uint16_t panId;
    uint16_t shortAddr;
    uint8_t extAddr[WHAL_IEEE802154_EXT_ADDR_SZ];
    bool promiscuous;
    bool rxOnWhenIdle;
} ZbPib;

typedef struct {
    bool enabled;
    uint32_t shortUsed;
    uint32_t extUsed;
    uint16_t shortAddr[ZB_PENDING_TABLE_SZ];
    uint8_t extAddr[ZB_PENDING_TABLE_SZ][WHAL_IEEE802154_EXT_ADDR_SZ];
} ZbPendingTable;

typedef struct {
    whal_Ieee802154_TxCfg cfg;
    bool ackRequested;
    uint8_t nb;
    uint8_t be;
    uint8_t retries;
} ZbTx;

static volatile ZbState s_state;
static ZbPib s_pib;
static ZbPendingTable s_pending;
static ZbTx s_tx;
static uint32_t s_rngState;

/* DMA buffers: byte 0 is the PHY header (frame length including FCS). The
 * receive buffer holds RSSI and LQI where the FCS would be. */
static uint8_t s_txBuf[1 + WHAL_IEEE802154_FRAME_MAX + 2];
static uint8_t s_rxBuf[1 + WHAL_IEEE802154_FRAME_MAX + 2];

static uint8_t s_phyCalData[PHY_CAL_DATA_SZ];

/* PHY init parameters: per-rate TX power limits for a 20 dBm maximum. */
static const uint8_t s_phyInitData[128] = {
    [0] = 0x01,
    [2] = 0x50, 0x50, 0x50, 0x50, 0x50, 0x4c, 0x4c, 0x4c, 0x4c, 0x48,
          0x28, 0x28, 0x28, 0x28, 0x4c, 0x4c, 0x4c, 0x4c, 0x48,
          0x28, 0x28, 0x28, 0x28,
    [28 ... 78] = 0xff,
    [111] = 0x9b,
};

static size_t Esp32c6_Ieee802154_IrqSave(void)
{
    size_t mstatus;

    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(mstatus) : "r"(1UL << 3) : "memory");

    return mstatus;
}

static void Esp32c6_Ieee802154_IrqRestore(size_t mstatus)
{
    __asm__ volatile("csrs mstatus, %0"
                     :: "r"(mstatus & (1UL << 3)) : "memory");
}

static uint32_t Esp32c6_Ieee802154_Random(void)
{
    uint32_t x = s_rngState;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_rngState = x;

    return x;
}

static void Esp32c6_Ieee802154_Cmd(size_t base, uint32_t cmd)
{
    whal_Reg_Write(base, ZB_COMMAND_REG, cmd);
}

static uint8_t Esp32c6_Ieee802154_PowerIndex(int8_t dbm)
{
    if (dbm >= ZB_TX_POWER_DBM_MAX) {
        return ZB_TX_POWER_IDX_MAX;
    }
    if (dbm <= ZB_TX_POWER_DBM_MIN) {
        return ZB_TX_POWER_IDX_MIN;
    }
    return (uint8_t)((dbm - ZB_TX_POWER_DBM_MIN) / 3 + ZB_TX_POWER_IDX_MIN);
}

static void Esp32c6_Ieee802154_ApplyPib(size_t base)
{
    static const uint8_t ccaModeMap[] = {
        [WHAL_IEEE802154_CCA_MODE_ED] = ZB_CCA_ED,
        [WHAL_IEEE802154_CCA_MODE_CARRIER] = ZB_CCA_CARRIER,
        [WHAL_IEEE802154_CCA_MODE_ED_AND_CARRIER] = ZB_CCA_CARRIER_AND_ED,
        [WHAL_IEEE802154_CCA_MODE_ED_OR_CARRIER] = ZB_CCA_CARRIER_OR_ED,
    };
    const uint8_t *ext = s_pib.extAddr;

    whal_Reg_Update(base, ZB_CHANNEL_REG, ZB_HOP_Msk,
                    whal_SetBits(ZB_HOP_Msk, ZB_HOP_Pos,
                                 ZB_CHANNEL_TO_HOP(s_pib.channel)));
    whal_Reg_Update(base, ZB_TX_POWER_REG, ZB_TX_POWER_Msk,
                    whal_SetBits(ZB_TX_POWER_Msk, ZB_TX_POWER_Pos,
                                 Esp32c6_Ieee802154_PowerIndex(s_pib.txPower)));
    whal_Reg_Update(base, ZB_ED_CFG_REG,
                    ZB_CCA_MODE_Msk | ZB_CCA_THRESHOLD_Msk,
                    whal_SetBits(ZB_CCA_MODE_Msk, ZB_CCA_MODE_Pos,
                                 ccaModeMap[s_pib.ccaMode]) |
                    whal_SetBits(ZB_CCA_THRESHOLD_Msk, ZB_CCA_THRESHOLD_Pos,
                                 (uint8_t)s_pib.ccaThreshold));

    whal_Reg_Write(base, ZB_INF0_PAN_ID_REG, s_pib.panId);
    whal_Reg_Write(base, ZB_INF0_SHORT_ADDR_REG, s_pib.shortAddr);
    whal_Reg_Write(base, ZB_INF0_EXT_ADDR0_REG,
                   (uint32_t)ext[0] | ((uint32_t)ext[1] << 8) |
                   ((uint32_t)ext[2] << 16) | ((uint32_t)ext[3] << 24));
    whal_Reg_Write(base, ZB_INF0_EXT_ADDR1_REG,
                   (uint32_t)ext[4] | ((uint32_t)ext[5] << 8) |
                   ((uint32_t)ext[6] << 16) | ((uint32_t)ext[7] << 24));

    whal_Reg_Update(base, ZB_CTRL_CFG_REG,
                    ZB_AUTO_ACK_TX_Msk | ZB_AUTO_ACK_RX_Msk |
                    ZB_COORDINATOR_Msk | ZB_PROMISCUOUS_Msk |
                    ZB_PENDING_ENHANCE_Msk | ZB_MULTIPAN_Msk,
                    ZB_AUTO_ACK_TX_Msk | ZB_AUTO_ACK_RX_Msk |
                    (s_pib.promiscuous ? ZB_PROMISCUOUS_Msk : 0) |
                    whal_SetBits(ZB_MULTIPAN_Msk, ZB_MULTIPAN_Pos, 1));
}

static void Esp32c6_Ieee802154_StartRx(size_t base)
{
    whal_Reg_Write(base, ZB_RXDMA_ADDR_REG, (uint32_t)(size_t)s_rxBuf);
    Esp32c6_Ieee802154_Cmd(base, ZB_CMD_RX_START);
    s_state = ZB_STATE_RX;
}

static void Esp32c6_Ieee802154_Stop(size_t base)
{
    Esp32c6_Ieee802154_Cmd(base, ZB_CMD_STOP);
    Esp32c6_Ieee802154_Cmd(base, ZB_CMD_TIMER0_STOP);
    Esp32c6_Ieee802154_Cmd(base, ZB_CMD_TIMER1_STOP);
    whal_Reg_Update(base, ZB_EVENT_EN_REG, ZB_EVT_TIMER0, 0);
    whal_Reg_Write(base, ZB_EVENT_STATUS_REG, ZB_EVT_ALL);
    s_state = ZB_STATE_IDLE;
}

static void Esp32c6_Ieee802154_Idle(size_t base)
{
    if (s_pib.rxOnWhenIdle) {
        Esp32c6_Ieee802154_StartRx(base);
    } else {
        s_state = ZB_STATE_IDLE;
    }
}

/* Return to the idle state after a callback, unless it started an
 * operation. */
static void Esp32c6_Ieee802154_Resume(size_t base)
{
    if (s_state == ZB_STATE_IDLE) {
        Esp32c6_Ieee802154_Idle(base);
    }
}

static void Esp32c6_Ieee802154_StartTimer(size_t base, size_t thresholdReg,
                                          uint32_t startCmd, uint32_t us)
{
    whal_Reg_Write(base, thresholdReg, us);
    Esp32c6_Ieee802154_Cmd(base, startCmd);
}

static void Esp32c6_Ieee802154_TxAttempt(size_t base)
{
    uint32_t slots;

    whal_Reg_Write(base, ZB_TXDMA_ADDR_REG, (uint32_t)(size_t)s_txBuf);
    if (s_tx.ackRequested) {
        whal_Reg_Write(base, ZB_RXDMA_ADDR_REG, (uint32_t)(size_t)s_rxBuf);
    }

    if (!s_tx.cfg.csmaCa) {
        Esp32c6_Ieee802154_Cmd(base, ZB_CMD_TX_START);
        s_state = ZB_STATE_TX;
        return;
    }

    slots = Esp32c6_Ieee802154_Random() & WHAL_BITMASK(s_tx.be);
    if (slots == 0) {
        whal_Reg_Update(base, ZB_ED_DURATION_REG, ZB_ED_DURATION_Msk,
                        ZB_CCA_SYMBOLS);
        Esp32c6_Ieee802154_Cmd(base, ZB_CMD_CCA_TX_START);
        s_state = ZB_STATE_TX_CCA;
    } else {
        Esp32c6_Ieee802154_StartTimer(base, ZB_TIME1_THRESHOLD_REG,
                                      ZB_CMD_TIMER1_START,
                                      slots * ZB_UNIT_BACKOFF_US);
        s_state = ZB_STATE_TX_BACKOFF;
    }
}

static void Esp32c6_Ieee802154_TxConfirm(size_t base,
                                         whal_Ieee802154_TxStatus status,
                                         const uint8_t *ack, size_t ackLen,
                                         const whal_Ieee802154_Callbacks *cb,
                                         void *ctx)
{
    s_state = ZB_STATE_IDLE;
    if (cb && cb->TxConfirm) {
        cb->TxConfirm(ctx, status, ack, ackLen);
    }
    Esp32c6_Ieee802154_Resume(base);
}

static void Esp32c6_Ieee802154_TxChannelBusy(size_t base,
                                             const whal_Ieee802154_Callbacks *cb,
                                             void *ctx)
{
    s_tx.nb++;
    s_tx.be = s_tx.be < s_tx.cfg.maxBe ? s_tx.be + 1 : s_tx.cfg.maxBe;
    if (s_tx.nb > s_tx.cfg.maxCsmaBackoffs) {
        Esp32c6_Ieee802154_TxConfirm(base,
                                     WHAL_IEEE802154_TX_CHANNEL_ACCESS_FAILURE,
                                     NULL, 0, cb, ctx);
        return;
    }
    Esp32c6_Ieee802154_TxAttempt(base);
}

static void Esp32c6_Ieee802154_TxNoAck(size_t base,
                                       const whal_Ieee802154_Callbacks *cb,
                                       void *ctx)
{
    Esp32c6_Ieee802154_Cmd(base, ZB_CMD_TIMER0_STOP);
    whal_Reg_Update(base, ZB_EVENT_EN_REG, ZB_EVT_TIMER0, 0);

    if (s_tx.retries < s_tx.cfg.maxFrameRetries) {
        s_tx.retries++;
        s_tx.nb = 0;
        s_tx.be = s_tx.cfg.minBe;
        Esp32c6_Ieee802154_TxAttempt(base);
        return;
    }
    Esp32c6_Ieee802154_TxConfirm(base, WHAL_IEEE802154_TX_NO_ACK, NULL, 0,
                                 cb, ctx);
}

/* Find the source address of a 2003/2006 frame. Returns its length (2 or 8)
 * or 0 if the frame carries none. */
static size_t Esp32c6_Ieee802154_SrcAddr(const uint8_t *frame, size_t len,
                                         const uint8_t **addr)
{
    uint16_t fcf;
    uint8_t dstMode;
    uint8_t srcMode;
    size_t off = 3;
    size_t srcLen;

    if (len < 3) {
        return 0;
    }

    fcf = (uint16_t)(frame[0] | (frame[1] << 8));
    dstMode = (fcf >> FCF_DST_MODE_Pos) & 0x3;
    srcMode = (fcf >> FCF_SRC_MODE_Pos) & 0x3;
    if (srcMode == FCF_ADDR_MODE_NONE) {
        return 0;
    }

    if (dstMode != FCF_ADDR_MODE_NONE) {
        off += 2 + (dstMode == FCF_ADDR_MODE_SHORT ? 2 : 8);
    }
    if (!(fcf & FCF_PAN_ID_COMP_Msk) || dstMode == FCF_ADDR_MODE_NONE) {
        off += 2;
    }

    srcLen = srcMode == FCF_ADDR_MODE_SHORT ? 2 : 8;
    if (off + srcLen > len) {
        return 0;
    }

    *addr = &frame[off];
    return srcLen;
}

static bool Esp32c6_Ieee802154_PendingLookup(const uint8_t *frame, size_t len)
{
    const uint8_t *addr;
    size_t addrLen;
    size_t i;

    if (!s_pending.enabled) {
        return true;
    }

    addrLen = Esp32c6_Ieee802154_SrcAddr(frame, len, &addr);
    if (addrLen == 2) {
        uint16_t shortAddr = (uint16_t)(addr[0] | (addr[1] << 8));

        for (i = 0; i < ZB_PENDING_TABLE_SZ; i++) {
            if ((s_pending.shortUsed & (1UL << i)) &&
                s_pending.shortAddr[i] == shortAddr) {
                return true;
            }
        }
    } else if (addrLen == 8) {
        for (i = 0; i < ZB_PENDING_TABLE_SZ; i++) {
            size_t j;

            if (!(s_pending.extUsed & (1UL << i))) {
                continue;
            }
            for (j = 0; j < WHAL_IEEE802154_EXT_ADDR_SZ; j++) {
                if (s_pending.extAddr[i][j] != addr[j]) {
                    break;
                }
            }
            if (j == WHAL_IEEE802154_EXT_ADDR_SZ) {
                return true;
            }
        }
    }

    return false;
}

static void Esp32c6_Ieee802154_RxDeliver(size_t base,
                                         const whal_Ieee802154_Callbacks *cb,
                                         void *ctx, bool ackFramePending)
{
    whal_Ieee802154_RxInfo rx;
    uint8_t phr = s_rxBuf[0] & 0x7F;

    s_state = ZB_STATE_IDLE;
    if (phr >= 3 && cb && cb->RxIndication) {
        rx.frame = &s_rxBuf[1];
        rx.len = phr - 2;
        rx.rssi = (int8_t)s_rxBuf[phr - 1];
        rx.lqi = s_rxBuf[phr];
        rx.ackFramePending = ackFramePending;
        cb->RxIndication(ctx, &rx);
    }
    Esp32c6_Ieee802154_Resume(base);
}

whal_Error whal_Esp32c6_Ieee802154_Init(whal_Ieee802154 *radioDev)
{
    const whal_Esp32c6_Ieee802154_Cfg *cfg =
        (const whal_Esp32c6_Ieee802154_Cfg *)whal_Esp32c6_Ieee802154_Dev.cfg;
    size_t base = whal_Esp32c6_Ieee802154_Dev.base;
    size_t i;
    (void)radioDev;

    ets_update_cpu_frequency(cfg->cpuFreqMhz);

    /* Bring the shared front end up idle rather than in Wi-Fi receive */
    phy_init_param_set(1);
    phy_bbpll_en_usb(true);
    register_chipv7_phy(s_phyInitData, s_phyCalData, PHY_RF_CAL_FULL);
    bt_bb_v2_init_cmplx(1);
    bt_bb_set_zb_tx_on_delay(ZB_BB_TX_ON_DELAY);

    s_pib.channel = ZB_CHANNEL_MIN;
    s_pib.txPower = ZB_TX_POWER_DBM_MAX;
    s_pib.ccaMode = WHAL_IEEE802154_CCA_MODE_ED;
    s_pib.ccaThreshold = -60;
    s_pib.panId = 0xFFFF;
    s_pib.shortAddr = 0xFFFF;
    for (i = 0; i < WHAL_IEEE802154_EXT_ADDR_SZ; i++) {
        s_pib.extAddr[i] = 0;
    }
    s_pib.promiscuous = false;
    s_pib.rxOnWhenIdle = false;
    s_pending.enabled = false;
    s_pending.shortUsed = 0;
    s_pending.extUsed = 0;

    s_rngState = 0x2545F491;
    if (cfg->eui64) {
        for (i = 0; i < WHAL_IEEE802154_EXT_ADDR_SZ; i++) {
            s_rngState = (s_rngState << 5) ^ (s_rngState >> 27) ^ cfg->eui64[i];
        }
    }

    whal_Reg_Write(base, ZB_EVENT_EN_REG, ZB_EVT_ALL & ~ZB_EVT_TIMER0);
    whal_Reg_Write(base, ZB_TX_ABORT_EN_REG,
                   ZB_ABORT_BIT(ZB_TX_ABORT_RX_ACK_TIMEOUT) |
                   ZB_ABORT_BIT(ZB_TX_ABORT_TX_COEX_BREAK) |
                   ZB_ABORT_BIT(ZB_TX_ABORT_TX_SECURITY_ERROR) |
                   ZB_ABORT_BIT(ZB_TX_ABORT_CCA_FAILED) |
                   ZB_ABORT_BIT(ZB_TX_ABORT_CCA_BUSY));
    whal_Reg_Write(base, ZB_RX_ABORT_EN_REG,
                   ZB_ABORT_BIT(ZB_RX_ABORT_TX_ACK_TIMEOUT) |
                   ZB_ABORT_BIT(ZB_RX_ABORT_TX_ACK_COEX_BREAK));
    whal_Reg_Update(base, ZB_ED_CFG_REG, ZB_ED_PWR_SEL_AVG_Msk,
                    ZB_ED_PWR_SEL_AVG_Msk);
    whal_Reg_Write(base, ZB_COEX_PTI_REG, ZB_COEX_PTI_DISABLED);
    whal_Reg_Update(base, ZB_SEC_CTRL_REG, ZB_SEC_EN_Msk, 0);
    ieee802154_txon_delay_set();

    Esp32c6_Ieee802154_Stop(base);
    Esp32c6_Ieee802154_ApplyPib(base);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_Deinit(whal_Ieee802154 *radioDev)
{
    size_t base = whal_Esp32c6_Ieee802154_Dev.base;
    size_t irq;
    (void)radioDev;

    irq = Esp32c6_Ieee802154_IrqSave();
    Esp32c6_Ieee802154_Stop(base);
    whal_Reg_Write(base, ZB_EVENT_EN_REG, 0);
    Esp32c6_Ieee802154_IrqRestore(irq);

    phy_close_rf();
    phy_xpd_tsens();

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_GetCaps(whal_Ieee802154 *radioDev,
                                           uint32_t *caps)
{
    (void)radioDev;

    *caps = WHAL_IEEE802154_CAP_AUTO_ACK | WHAL_IEEE802154_CAP_ACK_WAIT |
            WHAL_IEEE802154_CAP_CSMA_CA | WHAL_IEEE802154_CAP_RETRANSMIT |
            WHAL_IEEE802154_CAP_FRAME_FILTER |
            WHAL_IEEE802154_CAP_PROMISCUOUS |
            WHAL_IEEE802154_CAP_PENDING_TABLE |
            WHAL_IEEE802154_CAP_ENERGY_DETECT;

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_SetAttr(whal_Ieee802154 *radioDev,
                                           whal_Ieee802154_Attr attr,
                                           const whal_Ieee802154_AttrVal *val)
{
    size_t base = whal_Esp32c6_Ieee802154_Dev.base;
    size_t irq;
    size_t i;
    (void)radioDev;

    if (attr == WHAL_IEEE802154_ATTR_EUI64 ||
        attr == WHAL_IEEE802154_ATTR_RX_SENSITIVITY) {
        return WHAL_EINVAL;
    }

    if ((attr == WHAL_IEEE802154_ATTR_CHANNEL &&
         (val->u8 < ZB_CHANNEL_MIN || val->u8 > ZB_CHANNEL_MAX)) ||
        (attr == WHAL_IEEE802154_ATTR_CCA_MODE &&
         (val->u8 < WHAL_IEEE802154_CCA_MODE_ED ||
          val->u8 > WHAL_IEEE802154_CCA_MODE_ED_OR_CARRIER)) ||
        attr > WHAL_IEEE802154_ATTR_RX_SENSITIVITY) {
        return WHAL_ENOTSUP;
    }

    irq = Esp32c6_Ieee802154_IrqSave();

    switch (attr) {
    case WHAL_IEEE802154_ATTR_CHANNEL:
        s_pib.channel = val->u8;
        break;
    case WHAL_IEEE802154_ATTR_TX_POWER:
        s_pib.txPower = val->i8;
        break;
    case WHAL_IEEE802154_ATTR_CCA_MODE:
        s_pib.ccaMode = val->u8;
        break;
    case WHAL_IEEE802154_ATTR_CCA_ED_THRESHOLD:
        s_pib.ccaThreshold = val->i8;
        break;
    case WHAL_IEEE802154_ATTR_PAN_ID:
        s_pib.panId = val->u16;
        break;
    case WHAL_IEEE802154_ATTR_SHORT_ADDR:
        s_pib.shortAddr = val->u16;
        break;
    case WHAL_IEEE802154_ATTR_EXT_ADDR:
        for (i = 0; i < WHAL_IEEE802154_EXT_ADDR_SZ; i++) {
            s_pib.extAddr[i] = val->bytes[i];
        }
        break;
    case WHAL_IEEE802154_ATTR_PROMISCUOUS:
        s_pib.promiscuous = val->u8 != 0;
        break;
    case WHAL_IEEE802154_ATTR_RX_ON_WHEN_IDLE:
        s_pib.rxOnWhenIdle = val->u8 != 0;
        break;
    default:
        break;
    }

    if (s_state == ZB_STATE_IDLE || s_state == ZB_STATE_RX) {
        Esp32c6_Ieee802154_Stop(base);
        Esp32c6_Ieee802154_ApplyPib(base);
        Esp32c6_Ieee802154_Idle(base);
    }

    Esp32c6_Ieee802154_IrqRestore(irq);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_GetAttr(whal_Ieee802154 *radioDev,
                                           whal_Ieee802154_Attr attr,
                                           whal_Ieee802154_AttrVal *val)
{
    const whal_Esp32c6_Ieee802154_Cfg *cfg =
        (const whal_Esp32c6_Ieee802154_Cfg *)whal_Esp32c6_Ieee802154_Dev.cfg;
    size_t i;
    (void)radioDev;

    switch (attr) {
    case WHAL_IEEE802154_ATTR_CHANNEL:
        val->u8 = s_pib.channel;
        break;
    case WHAL_IEEE802154_ATTR_TX_POWER:
        val->i8 = s_pib.txPower;
        break;
    case WHAL_IEEE802154_ATTR_CCA_MODE:
        val->u8 = s_pib.ccaMode;
        break;
    case WHAL_IEEE802154_ATTR_CCA_ED_THRESHOLD:
        val->i8 = s_pib.ccaThreshold;
        break;
    case WHAL_IEEE802154_ATTR_PAN_ID:
        val->u16 = s_pib.panId;
        break;
    case WHAL_IEEE802154_ATTR_SHORT_ADDR:
        val->u16 = s_pib.shortAddr;
        break;
    case WHAL_IEEE802154_ATTR_EXT_ADDR:
        for (i = 0; i < WHAL_IEEE802154_EXT_ADDR_SZ; i++) {
            val->bytes[i] = s_pib.extAddr[i];
        }
        break;
    case WHAL_IEEE802154_ATTR_PROMISCUOUS:
        val->u8 = s_pib.promiscuous;
        break;
    case WHAL_IEEE802154_ATTR_RX_ON_WHEN_IDLE:
        val->u8 = s_pib.rxOnWhenIdle;
        break;
    case WHAL_IEEE802154_ATTR_EUI64:
        if (!cfg->eui64) {
            return WHAL_ENOTSUP;
        }
        for (i = 0; i < WHAL_IEEE802154_EXT_ADDR_SZ; i++) {
            val->bytes[i] = cfg->eui64[i];
        }
        break;
    case WHAL_IEEE802154_ATTR_RX_SENSITIVITY:
        val->i8 = ZB_RX_SENSITIVITY_DBM;
        break;
    default:
        return WHAL_ENOTSUP;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_Transmit(whal_Ieee802154 *radioDev,
                                            const uint8_t *frame, size_t len,
                                            const whal_Ieee802154_TxCfg *txCfg)
{
    size_t base = whal_Esp32c6_Ieee802154_Dev.base;
    size_t irq;
    size_t i;
    (void)radioDev;

    if (txCfg->csmaCa &&
        (txCfg->maxCsmaBackoffs > 5 || txCfg->maxBe < 3 || txCfg->maxBe > 8 ||
         txCfg->minBe > txCfg->maxBe)) {
        return WHAL_ENOTSUP;
    }
    if (txCfg->maxFrameRetries > 7) {
        return WHAL_ENOTSUP;
    }

    irq = Esp32c6_Ieee802154_IrqSave();

    if (s_state != ZB_STATE_IDLE && s_state != ZB_STATE_RX) {
        Esp32c6_Ieee802154_IrqRestore(irq);
        return WHAL_ENOTREADY;
    }

    Esp32c6_Ieee802154_Stop(base);
    Esp32c6_Ieee802154_ApplyPib(base);

    s_txBuf[0] = (uint8_t)(len + 2);
    for (i = 0; i < len; i++) {
        s_txBuf[1 + i] = frame[i];
    }

    s_tx.cfg = *txCfg;
    s_tx.ackRequested = (frame[0] & FCF_ACK_REQUEST_Msk) != 0;
    s_tx.nb = 0;
    s_tx.be = txCfg->minBe;
    s_tx.retries = 0;

    Esp32c6_Ieee802154_TxAttempt(base);

    Esp32c6_Ieee802154_IrqRestore(irq);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_GetRssi(whal_Ieee802154 *radioDev,
                                           int8_t *rssi)
{
    (void)radioDev;

    *rssi = (int8_t)(bt_bb_get_cur_rx_info() & 0xFF);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_Cca(whal_Ieee802154 *radioDev, bool *busy)
{
    (void)radioDev;
    (void)busy;

    return WHAL_ENOTSUP;
}

whal_Error whal_Esp32c6_Ieee802154_EnergyDetect(whal_Ieee802154 *radioDev,
                                                uint32_t durationUs)
{
    size_t base = whal_Esp32c6_Ieee802154_Dev.base;
    uint32_t symbols = durationUs / ZB_SYMBOL_US;
    size_t irq;
    (void)radioDev;

    if (symbols == 0 || symbols > WHAL_BITMASK(24)) {
        return WHAL_ENOTSUP;
    }

    irq = Esp32c6_Ieee802154_IrqSave();

    if (s_state != ZB_STATE_IDLE && s_state != ZB_STATE_RX) {
        Esp32c6_Ieee802154_IrqRestore(irq);
        return WHAL_ENOTREADY;
    }

    Esp32c6_Ieee802154_Stop(base);
    Esp32c6_Ieee802154_ApplyPib(base);
    whal_Reg_Update(base, ZB_ED_DURATION_REG, ZB_ED_DURATION_Msk, symbols);
    Esp32c6_Ieee802154_Cmd(base, ZB_CMD_ED_START);
    s_state = ZB_STATE_ED;

    Esp32c6_Ieee802154_IrqRestore(irq);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_PendingTableEnable(whal_Ieee802154 *radioDev,
                                                      bool enable)
{
    (void)radioDev;

    s_pending.enabled = enable;

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_PendingTableAddShort(whal_Ieee802154 *radioDev,
                                                        uint16_t addr)
{
    size_t irq;
    size_t i;
    size_t freeSlot = ZB_PENDING_TABLE_SZ;
    whal_Error err = WHAL_SUCCESS;
    (void)radioDev;

    irq = Esp32c6_Ieee802154_IrqSave();

    for (i = 0; i < ZB_PENDING_TABLE_SZ; i++) {
        if (!(s_pending.shortUsed & (1UL << i))) {
            if (freeSlot == ZB_PENDING_TABLE_SZ) {
                freeSlot = i;
            }
        } else if (s_pending.shortAddr[i] == addr) {
            break;
        }
    }

    if (i == ZB_PENDING_TABLE_SZ) {
        if (freeSlot == ZB_PENDING_TABLE_SZ) {
            err = WHAL_ENOTREADY;
        } else {
            s_pending.shortAddr[freeSlot] = addr;
            s_pending.shortUsed |= 1UL << freeSlot;
        }
    }

    Esp32c6_Ieee802154_IrqRestore(irq);

    return err;
}

whal_Error whal_Esp32c6_Ieee802154_PendingTableAddExtended(whal_Ieee802154 *radioDev,
                                                           const uint8_t *addr)
{
    size_t irq;
    size_t i;
    size_t j;
    size_t freeSlot = ZB_PENDING_TABLE_SZ;
    whal_Error err = WHAL_SUCCESS;
    (void)radioDev;

    irq = Esp32c6_Ieee802154_IrqSave();

    for (i = 0; i < ZB_PENDING_TABLE_SZ; i++) {
        if (!(s_pending.extUsed & (1UL << i))) {
            if (freeSlot == ZB_PENDING_TABLE_SZ) {
                freeSlot = i;
            }
            continue;
        }
        for (j = 0; j < WHAL_IEEE802154_EXT_ADDR_SZ; j++) {
            if (s_pending.extAddr[i][j] != addr[j]) {
                break;
            }
        }
        if (j == WHAL_IEEE802154_EXT_ADDR_SZ) {
            break;
        }
    }

    if (i == ZB_PENDING_TABLE_SZ) {
        if (freeSlot == ZB_PENDING_TABLE_SZ) {
            err = WHAL_ENOTREADY;
        } else {
            for (j = 0; j < WHAL_IEEE802154_EXT_ADDR_SZ; j++) {
                s_pending.extAddr[freeSlot][j] = addr[j];
            }
            s_pending.extUsed |= 1UL << freeSlot;
        }
    }

    Esp32c6_Ieee802154_IrqRestore(irq);

    return err;
}

whal_Error whal_Esp32c6_Ieee802154_PendingTableRemoveShort(whal_Ieee802154 *radioDev,
                                                           uint16_t addr)
{
    size_t irq;
    size_t i;
    (void)radioDev;

    irq = Esp32c6_Ieee802154_IrqSave();

    for (i = 0; i < ZB_PENDING_TABLE_SZ; i++) {
        if ((s_pending.shortUsed & (1UL << i)) &&
            s_pending.shortAddr[i] == addr) {
            s_pending.shortUsed &= ~(1UL << i);
        }
    }

    Esp32c6_Ieee802154_IrqRestore(irq);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_PendingTableRemoveExtended(whal_Ieee802154 *radioDev,
                                                              const uint8_t *addr)
{
    size_t irq;
    size_t i;
    size_t j;
    (void)radioDev;

    irq = Esp32c6_Ieee802154_IrqSave();

    for (i = 0; i < ZB_PENDING_TABLE_SZ; i++) {
        if (!(s_pending.extUsed & (1UL << i))) {
            continue;
        }
        for (j = 0; j < WHAL_IEEE802154_EXT_ADDR_SZ; j++) {
            if (s_pending.extAddr[i][j] != addr[j]) {
                break;
            }
        }
        if (j == WHAL_IEEE802154_EXT_ADDR_SZ) {
            s_pending.extUsed &= ~(1UL << i);
        }
    }

    Esp32c6_Ieee802154_IrqRestore(irq);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Ieee802154_PendingTableClear(whal_Ieee802154 *radioDev)
{
    (void)radioDev;

    s_pending.shortUsed = 0;
    s_pending.extUsed = 0;

    return WHAL_SUCCESS;
}

void whal_Esp32c6_Ieee802154_IrqHandler(whal_Ieee802154 *radioDev,
                                        const whal_Ieee802154_Callbacks *cb,
                                        void *ctx)
{
    size_t base = whal_Esp32c6_Ieee802154_Dev.base;
    uint32_t events;
    uint32_t rxAbort;
    uint32_t txAbort;
    (void)radioDev;

    events = whal_Reg_Read(base, ZB_EVENT_STATUS_REG) & ZB_EVT_ALL;
    rxAbort = whal_GetBits(ZB_RX_ABORT_Msk, ZB_RX_ABORT_Pos,
                           whal_Reg_Read(base, ZB_RX_STATUS_REG));
    txAbort = whal_GetBits(ZB_TX_ABORT_Msk, ZB_TX_ABORT_Pos,
                           whal_Reg_Read(base, ZB_TX_STATUS_REG));
    whal_Reg_Write(base, ZB_EVENT_STATUS_REG, events);

    if ((events & ZB_EVT_TIMER1) && s_state == ZB_STATE_TX_BACKOFF) {
        whal_Reg_Update(base, ZB_ED_DURATION_REG, ZB_ED_DURATION_Msk,
                        ZB_CCA_SYMBOLS);
        Esp32c6_Ieee802154_Cmd(base, ZB_CMD_CCA_TX_START);
        s_state = ZB_STATE_TX_CCA;
    }

    if ((events & ZB_EVT_TX_DONE) &&
        (s_state == ZB_STATE_TX || s_state == ZB_STATE_TX_CCA)) {
        if (s_tx.ackRequested) {
            whal_Reg_Update(base, ZB_EVENT_EN_REG, ZB_EVT_TIMER0,
                            ZB_EVT_TIMER0);
            Esp32c6_Ieee802154_StartTimer(base, ZB_TIME0_THRESHOLD_REG,
                                          ZB_CMD_TIMER0_START,
                                          ZB_ACK_WAIT_LIMIT_US);
            s_state = ZB_STATE_RX_ACK;
        } else {
            Esp32c6_Ieee802154_TxConfirm(base, WHAL_IEEE802154_TX_SUCCESS,
                                         NULL, 0, cb, ctx);
        }
    }

    if ((events & ZB_EVT_ACK_RX_DONE) && s_state == ZB_STATE_RX_ACK) {
        uint8_t phr = s_rxBuf[0] & 0x7F;

        Esp32c6_Ieee802154_Cmd(base, ZB_CMD_TIMER0_STOP);
        whal_Reg_Update(base, ZB_EVENT_EN_REG, ZB_EVT_TIMER0, 0);
        Esp32c6_Ieee802154_TxConfirm(base, WHAL_IEEE802154_TX_SUCCESS,
                                     &s_rxBuf[1], phr >= 2 ? phr - 2 : 0,
                                     cb, ctx);
    }

    if ((events & ZB_EVT_RX_DONE) && s_state == ZB_STATE_RX) {
        const uint8_t *frame = &s_rxBuf[1];
        uint8_t phr = s_rxBuf[0] & 0x7F;
        uint16_t fcf = phr >= 4 ? (uint16_t)(frame[0] | (frame[1] << 8)) : 0;

        if ((fcf & FCF_ACK_REQUEST_Msk) &&
            ((fcf >> FCF_VERSION_Pos) & 0x3) <= FCF_VERSION_2006 &&
            (fcf & FCF_FRAME_TYPE_Msk) != FCF_FRAME_TYPE_ACK) {
            whal_Reg_Update(base, ZB_ACK_PENDING_REG, ZB_ACK_PENDING_Msk,
                            Esp32c6_Ieee802154_PendingLookup(frame, phr - 2) ?
                            ZB_ACK_PENDING_Msk : 0);
            s_state = ZB_STATE_TX_ACK;
        } else {
            Esp32c6_Ieee802154_RxDeliver(base, cb, ctx, false);
        }
    }

    if ((events & ZB_EVT_ACK_TX_DONE) && s_state == ZB_STATE_TX_ACK) {
        Esp32c6_Ieee802154_RxDeliver(base, cb, ctx,
                                     whal_Reg_Read(base, ZB_ACK_PENDING_REG) &
                                     ZB_ACK_PENDING_Msk);
    }

    if (events & ZB_EVT_RX_ABORT) {
        if (s_state == ZB_STATE_TX_ACK &&
            (rxAbort == ZB_RX_ABORT_TX_ACK_TIMEOUT ||
             rxAbort == ZB_RX_ABORT_TX_ACK_COEX_BREAK)) {
            Esp32c6_Ieee802154_RxDeliver(base, cb, ctx, false);
        } else if (s_state == ZB_STATE_RX) {
            Esp32c6_Ieee802154_StartRx(base);
        }
    }

    if (events & ZB_EVT_TX_ABORT) {
        if (s_state == ZB_STATE_TX_CCA &&
            (txAbort == ZB_TX_ABORT_CCA_BUSY ||
             txAbort == ZB_TX_ABORT_CCA_FAILED)) {
            Esp32c6_Ieee802154_TxChannelBusy(base, cb, ctx);
        } else if (s_state == ZB_STATE_RX_ACK &&
                   ((txAbort >= ZB_TX_ABORT_RX_ACK_SFD_TIMEOUT &&
                     txAbort <= ZB_TX_ABORT_RX_ACK_RESTART) ||
                    txAbort == ZB_TX_ABORT_RX_ACK_TIMEOUT)) {
            Esp32c6_Ieee802154_TxNoAck(base, cb, ctx);
        } else if ((s_state == ZB_STATE_TX || s_state == ZB_STATE_TX_CCA) &&
                   (txAbort == ZB_TX_ABORT_TX_COEX_BREAK ||
                    txAbort == ZB_TX_ABORT_TX_SECURITY_ERROR)) {
            Esp32c6_Ieee802154_TxConfirm(base,
                                         WHAL_IEEE802154_TX_CHANNEL_ACCESS_FAILURE,
                                         NULL, 0, cb, ctx);
        }
    }

    if ((events & ZB_EVT_TIMER0) && s_state == ZB_STATE_RX_ACK) {
        Esp32c6_Ieee802154_Cmd(base, ZB_CMD_STOP);
        Esp32c6_Ieee802154_TxNoAck(base, cb, ctx);
    }

    if ((events & ZB_EVT_ED_DONE) && s_state == ZB_STATE_ED) {
        int8_t rss = (int8_t)whal_GetBits(ZB_ED_RSS_Msk, ZB_ED_RSS_Pos,
                                          whal_Reg_Read(base, ZB_ED_CFG_REG));

        s_state = ZB_STATE_IDLE;
        if (cb && cb->EdConfirm) {
            cb->EdConfirm(ctx, rss);
        }
        Esp32c6_Ieee802154_Resume(base);
    }
}

const whal_Ieee802154Driver whal_Esp32c6_Ieee802154_Driver = {
    .Init = whal_Esp32c6_Ieee802154_Init,
    .Deinit = whal_Esp32c6_Ieee802154_Deinit,
    .GetCaps = whal_Esp32c6_Ieee802154_GetCaps,
    .SetAttr = whal_Esp32c6_Ieee802154_SetAttr,
    .GetAttr = whal_Esp32c6_Ieee802154_GetAttr,
    .Transmit = whal_Esp32c6_Ieee802154_Transmit,
    .GetRssi = whal_Esp32c6_Ieee802154_GetRssi,
    .Cca = whal_Esp32c6_Ieee802154_Cca,
    .EnergyDetect = whal_Esp32c6_Ieee802154_EnergyDetect,
    .PendingTableEnable = whal_Esp32c6_Ieee802154_PendingTableEnable,
    .PendingTableAddShort = whal_Esp32c6_Ieee802154_PendingTableAddShort,
    .PendingTableAddExtended = whal_Esp32c6_Ieee802154_PendingTableAddExtended,
    .PendingTableRemoveShort = whal_Esp32c6_Ieee802154_PendingTableRemoveShort,
    .PendingTableRemoveExtended = whal_Esp32c6_Ieee802154_PendingTableRemoveExtended,
    .PendingTableClear = whal_Esp32c6_Ieee802154_PendingTableClear,
};

/* Symbols the PHY and baseband libraries expect from ESP-IDF. */

int phy_printf(const char *format, ...)
{
    (void)format;

    return 0;
}

int rtc_clk_xtal_freq_get(void)
{
    return 40;
}
