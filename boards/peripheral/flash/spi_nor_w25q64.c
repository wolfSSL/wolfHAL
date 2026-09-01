/* spi_nor_w25q64.c
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

#include "spi_nor_w25q64.h"
#include <wolfHAL/flash/spi_nor_flash.h>
#include "wolfHAL_board.h"

/*
 * Winbond W25Q64 — 64 Mbit (8 MB) SPI-NOR Flash
 *
 * - Page size:   256 bytes
 * - Sector size: 4 KB (smallest erasable unit)
 * - Capacity:    8,388,608 bytes (8 MB)
 * - SPI modes 0 and 3, up to 104 MHz (standard read up to 50 MHz)
 */

#define W25Q64_PAGE_SZ  256
#define W25Q64_CAPACITY (8 * 1024 * 1024) /* 8 MB */

static whal_Spi_ComCfg g_w25q64ComCfg = {
    .freq = 25000000, /* 25 MHz */
    .mode = WHAL_SPI_MODE_0,
    .wordSz = 8,
    .dataLines = 1,
};

whal_Flash g_whalSpiNorW25q64 = {
    .driver = &whal_SpiNor_Driver,
    .cfg = &(whal_SpiNor_Cfg) {
        .spiDev = BOARD_SPI_DEV,
        .spiComCfg = &g_w25q64ComCfg,
        .gpioDev = BOARD_GPIO_DEV,
        .csPin = SPI_CS_PIN,
        .timeout = &g_whalTimeout,
        .pageSz = W25Q64_PAGE_SZ,
        .capacity = W25Q64_CAPACITY,
    },
};
