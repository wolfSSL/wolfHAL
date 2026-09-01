/* sharp_memory_display.h
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

#ifndef WHAL_SHARPMEMORY_DISPLAY_H
#define WHAL_SHARPMEMORY_DISPLAY_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/display/display.h>
#include <wolfHAL/spi/spi.h>
#include <wolfHAL/gpio/gpio.h>
#include <wolfHAL/pwm/pwm.h>

/**
 * @file sharp_memory_display.h
 * @brief Sharp Memory LCD driver (LS013B7DH03 and compatible).
 *
 * Drives a Sharp memory-in-pixel monochrome LCD over a 3-wire SPI bus
 * (SCLK, SI, SCS). The panel has no read path and no register file: each
 * transaction is a one-shot serial frame that either updates whole display
 * lines, clears the panel, or holds the current image.
 *
 * Wire format notes (handled by the driver):
 *  - SCS (chip select) is *active high*, unlike a conventional SPI slave,
 *    and is driven as a plain GPIO (cfg->gpioDev / cfg->csPin).
 *  - The bus is mode 0 and runs at up to 1.1 MHz (set via cfg->spiComCfg).
 *  - COM inversion (VCOM) is performed in hardware: the board ties EXTMODE
 *    high and feeds EXTCOMIN from a ~1-60 Hz, 50%-duty signal. When a vcomCfg
 *    waveform is supplied the driver starts/stops cfg->pwm as that source.
 *
 * Pixel data format for whal_SharpMemory_Display_Update():
 *  - 1 bit per pixel, packed MSB-first: bit 7 of the first byte is the
 *    left-most pixel of a line, bit 6 the next, and so on.
 *  - A bit value of 1 is white, 0 is black (matching the panel's "H=white").
 *  - The panel can only write whole lines, so an update must span the full
 *    panel width (x == 0, w == width); any contiguous range of rows may be
 *    updated. @p data is therefore h * (width / 8) bytes, row-major.
 */

/**
 * @brief Display device configuration.
 */
typedef struct whal_SharpMemory_Display_Cfg {
    whal_Spi *spiDev;            /**< SPI bus device */
    whal_Spi_ComCfg *spiComCfg;  /**< SPI session config for StartCom */
    whal_Gpio *gpioDev;          /**< GPIO device for chip select (SCS);
                                  *   may be WHAL_INTERNAL_DEV on platforms
                                  *   that direct-map the GPIO API */
    size_t csPin;                /**< GPIO pin index for SCS (active high) */
    whal_Pwm *pwm;               /**< PWM driving EXTCOMIN (used only when
                                  *   vcomCfg is set) */
    uint8_t pwmChannel;          /**< PWM channel driving EXTCOMIN */
    whal_Pwm_ChannelCfg *vcomCfg;/**< EXTCOMIN waveform, and the presence flag
                                  *   for the PWM VCOM source: NULL means the
                                  *   board generates VCOM elsewhere and the
                                  *   driver starts no PWM (the device handles
                                  *   may be WHAL_INTERNAL_DEV/NULL, so they
                                  *   cannot serve as the discriminator) */
    uint16_t width;              /**< Panel width in pixels (128) */
    uint16_t height;             /**< Panel height in pixels (128); at most
                                  *   255, the range of the gate address */
} whal_SharpMemory_Display_Cfg;

#ifdef WHAL_CFG_SHARPMEMORY_DISPLAY_SINGLE_INSTANCE
/**
 * @brief Fixed device instance for boards with a single on-board panel.
 * Defined in the driver TU from the WHAL_CFG_SHARPMEMORY_DISPLAY_DEV
 * initializer in wolfHAL_board.h. When this macro is not defined, the driver
 * instead operates on the whal_Display passed to each call (e.g. an external
 * panel wired up under boards/peripheral/), like the SPI-NOR flash driver.
 */
extern const whal_Display whal_SharpMemory_Display_Dev;
#endif

#ifndef WHAL_CFG_SHARPMEMORY_DISPLAY_DIRECT_API_MAPPING
/**
 * @brief Driver instance for SharpMemory Display peripheral.
 */
extern const whal_DisplayDriver whal_SharpMemory_Display_Driver;

/**
 * @brief Initialize the SharpMemory display hardware.
 *
 * Deasserts SCS, opens the SPI session parameters, starts the EXTCOMIN
 * (VCOM) source when a PWM is configured, and clears the panel to white.
 *
 * @param dev Pointer to the display instance to initialize.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Null pointer or invalid configuration.
 * @retval WHAL_ENOTSUP Panel height exceeds 255 lines; the update frame
 *                      carries a single-byte gate address, so taller panels
 *                      need a wire format this driver does not implement.
 */
whal_Error whal_SharpMemory_Display_Init(whal_Display *dev);
/**
 * @brief Deinitialize the SharpMemory display hardware.
 *
 * Blanks the panel (clears pixel memory to white) and then stops the EXTCOMIN
 * source, so no static image is left latched without COM inversion.
 *
 * @param dev Pointer to the display instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Deinitialization completed.
 * @retval WHAL_EINVAL  Null pointer or invalid configuration.
 */
whal_Error whal_SharpMemory_Display_Deinit(whal_Display *dev);
/**
 * @brief Push pixel data to full-width display lines.
 *
 * The panel updates whole lines only, so the region must span the full
 * panel width (@p x == 0, @p w == width). Rows @p y .. @p y + @p h - 1 are
 * rewritten from @p data, which is @p h * (width / 8) bytes of 1bpp,
 * MSB-first pixel data (see the file header for the bit layout).
 *
 * @param dev    Pointer to the display instance.
 * @param x      Left column of the region (must be 0).
 * @param y      Top row of the region.
 * @param w      Width of the region in pixels (must equal the panel width).
 * @param h      Height of the region in pixels (number of lines).
 * @param data   Pointer to the pixel data to push.
 * @param dataSz Size of @p data in bytes; must be h * (width / 8).
 *
 * @retval WHAL_SUCCESS Region updated successfully.
 * @retval WHAL_EINVAL  Null pointer or malformed region request.
 * @retval WHAL_ENOTSUP Region not a full-width line range.
 */
whal_Error whal_SharpMemory_Display_Update(whal_Display *dev, uint16_t x, uint16_t y,
                                           uint16_t w, uint16_t h,
                                           const void *data, size_t dataSz);
#endif /* !WHAL_CFG_SHARPMEMORY_DISPLAY_DIRECT_API_MAPPING */

#endif /* WHAL_SHARPMEMORY_DISPLAY_H */
