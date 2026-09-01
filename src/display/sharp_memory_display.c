/* sharp_memory_display.c
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
#include <stddef.h>
#ifdef WHAL_CFG_SHARPMEMORY_DISPLAY_SINGLE_INSTANCE
#include "wolfHAL_board.h"  /* provides WHAL_CFG_SHARPMEMORY_DISPLAY_DEV initializer */
#endif
#include <wolfHAL/display/sharp_memory_display.h>
#include <wolfHAL/display/display.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/pwm/pwm.h>
#include <wolfHAL/error.h>

/*
 * Sharp Memory LCD serial command bits (mode-selection byte).
 *
 * On the wire the panel clocks LSB-first: M0 is the first bit, then M1, then
 * M2. The wolfHAL SPI bus transmits MSB-first, so the first wire bit must sit
 * in bit 7 of the transmitted byte. These constants are therefore already
 * expressed in that MSB-first orientation (M0 -> bit 7, M1 -> bit 6,
 * M2 -> bit 5).
 */
#define MODE_WRITE 0x80 /* M0=H: data update mode (write line memory)   */
#define MODE_CLEAR 0x20 /* M2=H: clear all pixel memory to white        */

#define DUMMY 0x00 /* trailing / don't-care byte ("L" recommended) */

#ifdef WHAL_CFG_SHARPMEMORY_DISPLAY_DIRECT_API_MAPPING
#define whal_SharpMemory_Display_Init    whal_Display_Init
#define whal_SharpMemory_Display_Deinit  whal_Display_Deinit
#define whal_SharpMemory_Display_Update  whal_Display_Update
#endif /* WHAL_CFG_SHARPMEMORY_DISPLAY_DIRECT_API_MAPPING */

#ifdef WHAL_CFG_SHARPMEMORY_DISPLAY_SINGLE_INSTANCE
const whal_Display whal_SharpMemory_Display_Dev = WHAL_CFG_SHARPMEMORY_DISPLAY_DEV;
#endif

/* SCS is active high on Sharp memory LCDs. */
static whal_Error SharpMem_CsAssert(whal_SharpMemory_Display_Cfg *cfg)
{
    return whal_Gpio_Set(cfg->gpioDev, cfg->csPin, 1);
}

static whal_Error SharpMem_CsDeassert(whal_SharpMemory_Display_Cfg *cfg)
{
    return whal_Gpio_Set(cfg->gpioDev, cfg->csPin, 0);
}

/*
 * Reverse the bit order of a byte. The gate-line address is sent LSB-first
 * (AG0 = LSB of the line number) over the MSB-first bus, so the line number
 * must be bit-reversed before transmission.
 */
static uint8_t SharpMem_Reverse8(uint8_t b)
{
    b = (uint8_t)(((b & 0xF0) >> 4) | ((b & 0x0F) << 4));
    b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
    b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
    return b;
}

/* Clear all pixel memory to white (all-clear frame, M2=H). */
static whal_Error SharpMem_Clear(whal_SharpMemory_Display_Cfg *cfg)
{
    uint8_t frame[2];
    whal_Error err;

    frame[0] = MODE_CLEAR;
    frame[1] = DUMMY;

    err = SharpMem_CsAssert(cfg);
    if (err)
        return err;
    err = whal_Spi_SendRecv(cfg->spiDev, frame, sizeof(frame), NULL, 0);
    SharpMem_CsDeassert(cfg);
    return err;
}

whal_Error whal_SharpMemory_Display_Init(whal_Display *dev)
{
    whal_SharpMemory_Display_Cfg *cfg;
    whal_Error err;
    int pwmStarted = 0;

#ifdef WHAL_CFG_SHARPMEMORY_DISPLAY_SINGLE_INSTANCE
    cfg = (whal_SharpMemory_Display_Cfg *)whal_SharpMemory_Display_Dev.cfg;
    (void)dev;
#else
    if (!dev || !dev->cfg)
        return WHAL_EINVAL;
    cfg = (whal_SharpMemory_Display_Cfg *)dev->cfg;
#endif

    /*
     * gpioDev may legitimately be WHAL_INTERNAL_DEV (NULL) on platforms that
     * direct-map the GPIO API, so it is not null-checked here; whal_Gpio_Set
     * validates it on vtable-dispatched platforms.
     */
    if (!cfg || !cfg->spiComCfg)
        return WHAL_EINVAL;
    if (cfg->width == 0 || cfg->height == 0 || (cfg->width & 0x7))
        return WHAL_EINVAL;
    if (cfg->height > 255)
        return WHAL_ENOTSUP;

    /* Ensure SCS starts deasserted (low) before configuring the bus. */
    err = SharpMem_CsDeassert(cfg);
    if (err)
        return err;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    /*
     * Start the EXTCOMIN (VCOM) source. COM polarity must be inverted
     * periodically to avoid a DC bias building up on the panel; the board
     * ties EXTMODE high and feeds EXTCOMIN from this signal.
     */
    if (cfg->vcomCfg) {
        err = whal_Pwm_Start(cfg->pwm, cfg->pwmChannel, cfg->vcomCfg);
        if (err)
            goto cleanup;
        pwmStarted = 1;
    }

    /* Power-on requires initializing pixel memory at least once. */
    err = SharpMem_Clear(cfg);

cleanup:
    if (err && pwmStarted)
        whal_Pwm_Stop(cfg->pwm, cfg->pwmChannel);
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

whal_Error whal_SharpMemory_Display_Deinit(whal_Display *dev)
{
    whal_SharpMemory_Display_Cfg *cfg;
    whal_Error err = WHAL_SUCCESS;

#ifdef WHAL_CFG_SHARPMEMORY_DISPLAY_SINGLE_INSTANCE
    cfg = (whal_SharpMemory_Display_Cfg *)whal_SharpMemory_Display_Dev.cfg;
    (void)dev;
#else
    if (!dev || !dev->cfg)
        return WHAL_EINVAL;
    cfg = (whal_SharpMemory_Display_Cfg *)dev->cfg;
#endif

    if (!cfg)
        return WHAL_EINVAL;

    /* Blank the panel before halting COM inversion so no static image is left
     * latched without EXTCOMIN toggling (DC bias). */
    if (cfg->spiComCfg) {
        err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
        if (!err) {
            err = SharpMem_Clear(cfg);
            whal_Spi_EndCom(cfg->spiDev);
        }
    }

    if (cfg->vcomCfg)
        whal_Pwm_Stop(cfg->pwm, cfg->pwmChannel);

    if (err == WHAL_SUCCESS)
        return SharpMem_CsDeassert(cfg);

    SharpMem_CsDeassert(cfg);
    return err;
}

whal_Error whal_SharpMemory_Display_Update(whal_Display *dev, uint16_t x, uint16_t y,
                                           uint16_t w, uint16_t h,
                                           const void *data, size_t dataSz)
{
    const uint8_t *pixels = (const uint8_t *)data;
    whal_SharpMemory_Display_Cfg *cfg;
    size_t bytesPerLine;
    uint8_t cmd = MODE_WRITE;
    uint8_t trailer = DUMMY;
    uint16_t line;
    whal_Error err;

#ifdef WHAL_CFG_SHARPMEMORY_DISPLAY_SINGLE_INSTANCE
    cfg = (whal_SharpMemory_Display_Cfg *)whal_SharpMemory_Display_Dev.cfg;
    (void)dev;
#else
    if (!dev || !dev->cfg)
        return WHAL_EINVAL;
    cfg = (whal_SharpMemory_Display_Cfg *)dev->cfg;
#endif

    if (!cfg || !cfg->spiComCfg)
        return WHAL_EINVAL;
    if (!data || h == 0)
        return WHAL_EINVAL;

    /* The panel writes whole lines: a region must span the full width. */
    if (x != 0 || w != cfg->width)
        return WHAL_ENOTSUP;
    if ((uint32_t)y + h > cfg->height)
        return WHAL_EINVAL;

    bytesPerLine = cfg->width / 8;
    if (dataSz != (size_t)h * bytesPerLine)
        return WHAL_EINVAL;

    err = whal_Spi_StartCom(cfg->spiDev, cfg->spiComCfg);
    if (err)
        return err;

    err = SharpMem_CsAssert(cfg);
    if (err)
        goto cleanup;

    /*
     * Frame layout (data update mode, M0=H):
     *   [mode] { [gate addr][line data][8ck dummy] }... [8ck dummy]
     * The trailing dummy of each line plus the final dummy form the 16ck
     * data-transfer period the panel needs to latch the last line.
     */
    err = whal_Spi_SendRecv(cfg->spiDev, &cmd, 1, NULL, 0);

    for (line = 0; !err && line < h; line++) {
        /* Gate lines are 1-based; address is sent LSB-first (bit-reversed). */
        uint8_t addr = SharpMem_Reverse8((uint8_t)(y + line + 1));

        err = whal_Spi_SendRecv(cfg->spiDev, &addr, 1, NULL, 0);
        if (!err)
            err = whal_Spi_SendRecv(cfg->spiDev,
                                    pixels + (size_t)line * bytesPerLine,
                                    bytesPerLine, NULL, 0);
        if (!err)
            err = whal_Spi_SendRecv(cfg->spiDev, &trailer, 1, NULL, 0);
    }

    /* Final dummy byte to complete the last line's transfer period. */
    if (!err)
        err = whal_Spi_SendRecv(cfg->spiDev, &trailer, 1, NULL, 0);

    SharpMem_CsDeassert(cfg);

cleanup:
    whal_Spi_EndCom(cfg->spiDev);
    return err;
}

#ifndef WHAL_CFG_SHARPMEMORY_DISPLAY_DIRECT_API_MAPPING
const whal_DisplayDriver whal_SharpMemory_Display_Driver = {
    .Init   = whal_SharpMemory_Display_Init,
    .Deinit = whal_SharpMemory_Display_Deinit,
    .Update = whal_SharpMemory_Display_Update,
};
#endif /* !WHAL_CFG_SHARPMEMORY_DISPLAY_DIRECT_API_MAPPING */
