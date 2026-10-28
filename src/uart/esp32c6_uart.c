/* esp32c6_uart.c
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
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/uart/esp32c6_uart.h>

#ifdef WHAL_CFG_ESP32C6_UART_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides WHAL_CFG_ESP32C6_UART_DEV initializer */
#endif

/* UART register offsets and fields (TRM chapter 27). Registers with a _SYNC
 * suffix are read in the UART core clock domain and only take effect after
 * a write to UART_REG_UPDATE. */

#define UART_FIFO_REG                   0x0000
#define UART_RXFIFO_RD_BYTE_Pos         0
#define UART_RXFIFO_RD_BYTE_Msk         (WHAL_BITMASK(8) << UART_RXFIFO_RD_BYTE_Pos)

/* INT_RAW and INT_CLR share one bit layout. */
#define UART_INT_RAW_REG                0x0004
#define UART_INT_CLR_REG                0x0010
#define UART_PARITY_ERR_INT_Pos         2
#define UART_PARITY_ERR_INT_Msk         (1UL << UART_PARITY_ERR_INT_Pos)
#define UART_FRM_ERR_INT_Pos            3
#define UART_FRM_ERR_INT_Msk            (1UL << UART_FRM_ERR_INT_Pos)
#define UART_RXFIFO_OVF_INT_Pos         4
#define UART_RXFIFO_OVF_INT_Msk         (1UL << UART_RXFIFO_OVF_INT_Pos)
#define UART_TX_DONE_INT_Pos            14
#define UART_TX_DONE_INT_Msk            (1UL << UART_TX_DONE_INT_Pos)
#define UART_INT_ERR_Msk                (UART_PARITY_ERR_INT_Msk | \
                                         UART_FRM_ERR_INT_Msk | \
                                         UART_RXFIFO_OVF_INT_Msk)

#define UART_CLKDIV_SYNC_REG            0x0014
#define UART_CLKDIV_Pos                 0
#define UART_CLKDIV_Msk                 (WHAL_BITMASK(12) << UART_CLKDIV_Pos)
#define UART_CLKDIV_FRAG_Pos            20
#define UART_CLKDIV_FRAG_Msk            (WHAL_BITMASK(4) << UART_CLKDIV_FRAG_Pos)

#define UART_STATUS_REG                 0x001C
#define UART_RXFIFO_CNT_Pos             0
#define UART_RXFIFO_CNT_Msk             (WHAL_BITMASK(8) << UART_RXFIFO_CNT_Pos)
#define UART_TXFIFO_CNT_Pos             16
#define UART_TXFIFO_CNT_Msk             (WHAL_BITMASK(8) << UART_TXFIFO_CNT_Pos)

#define UART_CONF0_SYNC_REG             0x0020
#define UART_PARITY_EN_Pos              1
#define UART_PARITY_EN_Msk              (1UL << UART_PARITY_EN_Pos)
#define UART_BIT_NUM_Pos                2
#define UART_BIT_NUM_Msk                (WHAL_BITMASK(2) << UART_BIT_NUM_Pos)
#define UART_STOP_BIT_NUM_Pos           4
#define UART_STOP_BIT_NUM_Msk           (WHAL_BITMASK(2) << UART_STOP_BIT_NUM_Pos)
#define UART_RXFIFO_RST_Pos             22
#define UART_RXFIFO_RST_Msk             (1UL << UART_RXFIFO_RST_Pos)
#define UART_TXFIFO_RST_Pos             23
#define UART_TXFIFO_RST_Msk             (1UL << UART_TXFIFO_RST_Pos)

/* BIT_NUM 3 selects 8 data bits; STOP_BIT_NUM 1 selects 1 stop bit. */
#define UART_BIT_NUM_8                  3
#define UART_STOP_BIT_NUM_1             1

#define UART_FSM_STATUS_REG             0x0070
#define UART_ST_UTX_OUT_Pos             4
#define UART_ST_UTX_OUT_Msk             (WHAL_BITMASK(4) << UART_ST_UTX_OUT_Pos)

#define UART_CLK_CONF_REG               0x0088
#define UART_TX_SCLK_EN_Pos             24
#define UART_TX_SCLK_EN_Msk             (1UL << UART_TX_SCLK_EN_Pos)
#define UART_RX_SCLK_EN_Pos             25
#define UART_RX_SCLK_EN_Msk             (1UL << UART_RX_SCLK_EN_Pos)

#define UART_REG_UPDATE_REG             0x0098
#define UART_REG_UPDATE_Pos             0
#define UART_REG_UPDATE_Msk             (1UL << UART_REG_UPDATE_Pos)

#define UART_FIFO_SZ                    128

#ifdef WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING
#define whal_Esp32c6_Uart_Init      whal_Uart_Init
#define whal_Esp32c6_Uart_Deinit    whal_Uart_Deinit
#define whal_Esp32c6_Uart_Send      whal_Uart_Send
#define whal_Esp32c6_Uart_Recv      whal_Uart_Recv
#define whal_Esp32c6_Uart_SendAsync whal_Uart_SendAsync
#define whal_Esp32c6_Uart_RecvAsync whal_Uart_RecvAsync
#endif /* WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_ESP32C6_UART_SINGLE_INSTANCE
const whal_Uart whal_Esp32c6_Uart_Dev = WHAL_CFG_ESP32C6_UART_DEV;
#endif

whal_Error whal_Esp32c6_Uart_Init(whal_Uart *uartDev)
{
    whal_Error err;
#ifdef WHAL_CFG_ESP32C6_UART_SINGLE_INSTANCE
    const whal_Esp32c6_Uart_Cfg *cfg =
        (const whal_Esp32c6_Uart_Cfg *)whal_Esp32c6_Uart_Dev.cfg;
    size_t base = whal_Esp32c6_Uart_Dev.base;
    (void)uartDev;
#else
    const whal_Esp32c6_Uart_Cfg *cfg;
    size_t base;

    if (!uartDev || !uartDev->cfg) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (const whal_Esp32c6_Uart_Cfg *)uartDev->cfg;
#endif

    whal_Reg_Update(base, UART_CLK_CONF_REG,
                    UART_TX_SCLK_EN_Msk | UART_RX_SCLK_EN_Msk,
                    UART_TX_SCLK_EN_Msk | UART_RX_SCLK_EN_Msk);

    /* Drain pending output before changing the baud rate. */
    err = whal_Reg_ReadPoll(base, UART_STATUS_REG, UART_TXFIFO_CNT_Msk, 0,
                            cfg->timeout);
    if (err) {
        return err;
    }
    err = whal_Reg_ReadPoll(base, UART_FSM_STATUS_REG, UART_ST_UTX_OUT_Msk, 0,
                            cfg->timeout);
    if (err) {
        return err;
    }
    err = whal_Reg_ReadPoll(base, UART_REG_UPDATE_REG, UART_REG_UPDATE_Msk, 0,
                            cfg->timeout);
    if (err) {
        return err;
    }

    whal_Reg_Update(base, UART_CLKDIV_SYNC_REG,
                    UART_CLKDIV_Msk | UART_CLKDIV_FRAG_Msk, cfg->clkDiv);
    whal_Reg_Update(base, UART_CONF0_SYNC_REG,
                    UART_PARITY_EN_Msk | UART_BIT_NUM_Msk |
                    UART_STOP_BIT_NUM_Msk | UART_RXFIFO_RST_Msk |
                    UART_TXFIFO_RST_Msk,
                    whal_SetBits(UART_BIT_NUM_Msk, UART_BIT_NUM_Pos,
                                 UART_BIT_NUM_8) |
                    whal_SetBits(UART_STOP_BIT_NUM_Msk, UART_STOP_BIT_NUM_Pos,
                                 UART_STOP_BIT_NUM_1) |
                    UART_RXFIFO_RST_Msk | UART_TXFIFO_RST_Msk);
    whal_Reg_Write(base, UART_REG_UPDATE_REG, UART_REG_UPDATE_Msk);
    err = whal_Reg_ReadPoll(base, UART_REG_UPDATE_REG, UART_REG_UPDATE_Msk, 0,
                            cfg->timeout);
    if (err) {
        return err;
    }

    whal_Reg_Update(base, UART_CONF0_SYNC_REG,
                    UART_RXFIFO_RST_Msk | UART_TXFIFO_RST_Msk, 0);
    whal_Reg_Write(base, UART_REG_UPDATE_REG, UART_REG_UPDATE_Msk);
    err = whal_Reg_ReadPoll(base, UART_REG_UPDATE_REG, UART_REG_UPDATE_Msk, 0,
                            cfg->timeout);
    if (err) {
        return err;
    }

    whal_Reg_Write(base, UART_INT_CLR_REG,
                   UART_INT_ERR_Msk | UART_TX_DONE_INT_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Uart_Deinit(whal_Uart *uartDev)
{
#ifdef WHAL_CFG_ESP32C6_UART_SINGLE_INSTANCE
    size_t base = whal_Esp32c6_Uart_Dev.base;
    (void)uartDev;
#else
    size_t base;

    if (!uartDev) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
#endif

    whal_Reg_Update(base, UART_CLK_CONF_REG,
                    UART_TX_SCLK_EN_Msk | UART_RX_SCLK_EN_Msk, 0);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Uart_Send(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    const uint8_t *buf = data;
    whal_Error err;
#ifdef WHAL_CFG_ESP32C6_UART_SINGLE_INSTANCE
    const whal_Esp32c6_Uart_Cfg *cfg =
        (const whal_Esp32c6_Uart_Cfg *)whal_Esp32c6_Uart_Dev.cfg;
    size_t base = whal_Esp32c6_Uart_Dev.base;
    (void)uartDev;

    if (!data) {
        return WHAL_EINVAL;
    }
#else
    const whal_Esp32c6_Uart_Cfg *cfg;
    size_t base;

    if (!uartDev || !uartDev->cfg || !data) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (const whal_Esp32c6_Uart_Cfg *)uartDev->cfg;
#endif

    if (dataSz == 0) {
        return WHAL_SUCCESS;
    }

    whal_Reg_Write(base, UART_INT_CLR_REG, UART_TX_DONE_INT_Msk);

    for (size_t i = 0; i < dataSz; ++i) {
        size_t cnt;

        WHAL_TIMEOUT_START(cfg->timeout);
        whal_Reg_Get(base, UART_STATUS_REG, UART_TXFIFO_CNT_Msk,
                     UART_TXFIFO_CNT_Pos, &cnt);
        while (cnt >= UART_FIFO_SZ) {
            if (WHAL_TIMEOUT_EXPIRED(cfg->timeout)) {
                return WHAL_ETIMEOUT;
            }
            whal_Reg_Get(base, UART_STATUS_REG, UART_TXFIFO_CNT_Msk,
                         UART_TXFIFO_CNT_Pos, &cnt);
        }

        whal_Reg_Write(base, UART_FIFO_REG, buf[i]);
    }

    /* TX_DONE alone can fire early if the FIFO drained mid-buffer. */
    err = whal_Reg_ReadPoll(base, UART_INT_RAW_REG, UART_TX_DONE_INT_Msk,
                            UART_TX_DONE_INT_Msk, cfg->timeout);
    if (err) {
        return err;
    }
    err = whal_Reg_ReadPoll(base, UART_STATUS_REG, UART_TXFIFO_CNT_Msk, 0,
                            cfg->timeout);
    if (err) {
        return err;
    }

    return whal_Reg_ReadPoll(base, UART_FSM_STATUS_REG, UART_ST_UTX_OUT_Msk, 0,
                             cfg->timeout);
}

whal_Error whal_Esp32c6_Uart_Recv(whal_Uart *uartDev, void *data, size_t dataSz)
{
    uint8_t *buf = data;
#ifdef WHAL_CFG_ESP32C6_UART_SINGLE_INSTANCE
    const whal_Esp32c6_Uart_Cfg *cfg =
        (const whal_Esp32c6_Uart_Cfg *)whal_Esp32c6_Uart_Dev.cfg;
    size_t base = whal_Esp32c6_Uart_Dev.base;
    (void)uartDev;

    if (!data) {
        return WHAL_EINVAL;
    }
#else
    const whal_Esp32c6_Uart_Cfg *cfg;
    size_t base;

    if (!uartDev || !uartDev->cfg || !data) {
        return WHAL_EINVAL;
    }

    base = uartDev->base;
    cfg = (const whal_Esp32c6_Uart_Cfg *)uartDev->cfg;
#endif
#ifdef WHAL_CFG_NO_TIMEOUT
    (void)cfg;
#endif

    for (size_t i = 0; i < dataSz; ++i) {
        size_t cnt;
        size_t d;
        size_t errBits;

        WHAL_TIMEOUT_START(cfg->timeout);
        whal_Reg_Get(base, UART_STATUS_REG, UART_RXFIFO_CNT_Msk,
                     UART_RXFIFO_CNT_Pos, &cnt);
        while (cnt == 0) {
            if (WHAL_TIMEOUT_EXPIRED(cfg->timeout)) {
                return WHAL_ETIMEOUT;
            }
            whal_Reg_Get(base, UART_STATUS_REG, UART_RXFIFO_CNT_Msk,
                         UART_RXFIFO_CNT_Pos, &cnt);
        }

        whal_Reg_Get(base, UART_FIFO_REG, UART_RXFIFO_RD_BYTE_Msk,
                     UART_RXFIFO_RD_BYTE_Pos, &d);
        buf[i] = d;

        errBits = whal_Reg_Read(base, UART_INT_RAW_REG) & UART_INT_ERR_Msk;
        if (errBits) {
            whal_Reg_Write(base, UART_INT_CLR_REG, errBits);
            return WHAL_EHARDWARE;
        }
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_Uart_SendAsync(whal_Uart *uartDev, const void *data, size_t dataSz)
{
    (void)uartDev;
    (void)dataSz;

    if (!data) {
        return WHAL_EINVAL;
    }

    return WHAL_ENOTSUP;
}

whal_Error whal_Esp32c6_Uart_RecvAsync(whal_Uart *uartDev, void *data, size_t dataSz)
{
    (void)uartDev;
    (void)dataSz;

    if (!data) {
        return WHAL_EINVAL;
    }

    return WHAL_ENOTSUP;
}

#ifndef WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING
const whal_UartDriver whal_Esp32c6_Uart_Driver = {
    .Init = whal_Esp32c6_Uart_Init,
    .Deinit = whal_Esp32c6_Uart_Deinit,
    .Send = whal_Esp32c6_Uart_Send,
    .Recv = whal_Esp32c6_Uart_Recv,
    .SendAsync = whal_Esp32c6_Uart_SendAsync,
    .RecvAsync = whal_Esp32c6_Uart_RecvAsync,
};
#endif /* !WHAL_CFG_ESP32C6_UART_DIRECT_API_MAPPING */
