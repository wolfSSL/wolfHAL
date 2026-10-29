/* esp32c6_usb_serial_jtag.c
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

#include "wolfHAL_board.h"  /* provides WHAL_CFG_ESP32C6_USB_SERIAL_JTAG_DEV initializer */
#include <stdint.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timeout.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/uart/esp32c6_usb_serial_jtag.h>

const whal_Uart whal_Esp32c6_UsbSerialJtag_Dev = WHAL_CFG_ESP32C6_USB_SERIAL_JTAG_DEV;

/* USB Serial/JTAG register offsets and fields (TRM chapter 32). */

#define USJ_EP1_REG                     0x0000
#define USJ_RDWR_BYTE_Pos               0
#define USJ_RDWR_BYTE_Msk               (WHAL_BITMASK(8) << USJ_RDWR_BYTE_Pos)

#define USJ_EP1_CONF_REG                0x0004
#define USJ_WR_DONE_Pos                 0
#define USJ_WR_DONE_Msk                 (1UL << USJ_WR_DONE_Pos)
#define USJ_IN_EP_DATA_FREE_Pos         1
#define USJ_IN_EP_DATA_FREE_Msk         (1UL << USJ_IN_EP_DATA_FREE_Pos)
#define USJ_OUT_EP_DATA_AVAIL_Pos       2
#define USJ_OUT_EP_DATA_AVAIL_Msk       (1UL << USJ_OUT_EP_DATA_AVAIL_Pos)

/* USB full-speed bulk packet size; the hardware flushes at this many bytes. */
#define USJ_PACKET_SZ                   64

whal_Error whal_Esp32c6_UsbSerialJtag_Init(whal_Uart *uartDev)
{
    (void)uartDev;

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_UsbSerialJtag_Deinit(whal_Uart *uartDev)
{
    (void)uartDev;

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_UsbSerialJtag_Send(whal_Uart *uartDev, const void *data,
                                           size_t dataSz)
{
    const whal_Esp32c6_UsbSerialJtag_Cfg *cfg =
        (const whal_Esp32c6_UsbSerialJtag_Cfg *)whal_Esp32c6_UsbSerialJtag_Dev.cfg;
    size_t base = whal_Esp32c6_UsbSerialJtag_Dev.base;
    const uint8_t *buf = data;
    (void)uartDev;
#ifdef WHAL_CFG_NO_TIMEOUT
    (void)cfg;
#endif

    if (!data) {
        return WHAL_EINVAL;
    }

    if (dataSz == 0) {
        return WHAL_SUCCESS;
    }

    for (size_t i = 0; i < dataSz; ++i) {
        WHAL_TIMEOUT_START(cfg->timeout);
        while (!(whal_Reg_Read(base, USJ_EP1_CONF_REG) &
                 USJ_IN_EP_DATA_FREE_Msk)) {
            if (WHAL_TIMEOUT_EXPIRED(cfg->timeout)) {
                return WHAL_ETIMEOUT;
            }
        }

        whal_Reg_Write(base, USJ_EP1_REG, buf[i]);
    }

    /* A full last packet was already flushed by hardware; flushing the empty
     * FIFO sends the zero-length packet that ends the host's transfer. */
    if (dataSz % USJ_PACKET_SZ == 0) {
        WHAL_TIMEOUT_START(cfg->timeout);
        while (!(whal_Reg_Read(base, USJ_EP1_CONF_REG) &
                 USJ_IN_EP_DATA_FREE_Msk)) {
            if (WHAL_TIMEOUT_EXPIRED(cfg->timeout)) {
                return WHAL_ETIMEOUT;
            }
        }
    }

    whal_Reg_Write(base, USJ_EP1_CONF_REG, USJ_WR_DONE_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_UsbSerialJtag_Recv(whal_Uart *uartDev, void *data,
                                           size_t dataSz)
{
    const whal_Esp32c6_UsbSerialJtag_Cfg *cfg =
        (const whal_Esp32c6_UsbSerialJtag_Cfg *)whal_Esp32c6_UsbSerialJtag_Dev.cfg;
    size_t base = whal_Esp32c6_UsbSerialJtag_Dev.base;
    uint8_t *buf = data;
    (void)uartDev;
#ifdef WHAL_CFG_NO_TIMEOUT
    (void)cfg;
#endif

    if (!data) {
        return WHAL_EINVAL;
    }

    for (size_t i = 0; i < dataSz; ++i) {
        size_t d;

        WHAL_TIMEOUT_START(cfg->timeout);
        while (!(whal_Reg_Read(base, USJ_EP1_CONF_REG) &
                 USJ_OUT_EP_DATA_AVAIL_Msk)) {
            if (WHAL_TIMEOUT_EXPIRED(cfg->timeout)) {
                return WHAL_ETIMEOUT;
            }
        }

        whal_Reg_Get(base, USJ_EP1_REG, USJ_RDWR_BYTE_Msk, USJ_RDWR_BYTE_Pos,
                     &d);
        buf[i] = (uint8_t)d;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Esp32c6_UsbSerialJtag_SendAsync(whal_Uart *uartDev,
                                                const void *data,
                                                size_t dataSz)
{
    (void)uartDev;
    (void)dataSz;

    if (!data) {
        return WHAL_EINVAL;
    }

    return WHAL_ENOTSUP;
}

whal_Error whal_Esp32c6_UsbSerialJtag_RecvAsync(whal_Uart *uartDev, void *data,
                                                size_t dataSz)
{
    (void)uartDev;
    (void)dataSz;

    if (!data) {
        return WHAL_EINVAL;
    }

    return WHAL_ENOTSUP;
}

const whal_UartDriver whal_Esp32c6_UsbSerialJtag_Driver = {
    .Init = whal_Esp32c6_UsbSerialJtag_Init,
    .Deinit = whal_Esp32c6_UsbSerialJtag_Deinit,
    .Send = whal_Esp32c6_UsbSerialJtag_Send,
    .Recv = whal_Esp32c6_UsbSerialJtag_Recv,
    .SendAsync = whal_Esp32c6_UsbSerialJtag_SendAsync,
    .RecvAsync = whal_Esp32c6_UsbSerialJtag_RecvAsync,
};
