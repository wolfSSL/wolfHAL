/* sdhc_spi_sdcard32gb.c
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

#include "sdhc_spi_sdcard32gb.h"
#include <wolfHAL/block/sdhc_spi_block.h>
#include "wolfHAL_board.h"

static whal_Spi_ComCfg g_sdcardComCfg = {
    .freq = 25000000, /* 25 MHz */
    .mode = WHAL_SPI_MODE_0,
    .wordSz = 8,
    .dataLines = 1,
};

whal_Block g_whalSdhcSpiSdcard32gb = {
    .driver = &whal_SdhcSpi_Driver,
    .cfg = &(whal_SdhcSpi_Cfg) {
        .spiDev = BOARD_SPI_DEV,
        .spiComCfg = &g_sdcardComCfg,
        .gpioDev = BOARD_GPIO_DEV,
        .csPin = SPI_CS_PIN,
        .timeout = &g_whalTimeout,
    },
};
