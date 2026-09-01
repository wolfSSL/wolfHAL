/* sharp_ls013b7dh03.c
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

#include "sharp_ls013b7dh03.h"
#include <wolfHAL/display/sharp_memory_display.h>
#include "wolfHAL_board.h"

/*
 * Sharp LS013B7DH03 - 128x128 monochrome memory LCD
 *
 * - 3-wire SPI (SCLK, SI, SCS), mode 0, up to 1.1 MHz
 * - SCS is active high and driven as a plain GPIO (board chip-select pin)
 * - COM inversion (VCOM) is supplied externally on EXTCOMIN with EXTMODE tied
 *   high; here it is driven from the board PWM at 60 Hz, 50% duty
 *
 * Wired to the board SPI bus and chip-select pin used for SPI peripherals.
 */

static whal_Spi_ComCfg g_ls013ComCfg = {
    .freq = 1000000, /* 1 MHz (panel max 1.1 MHz) */
    .mode = WHAL_SPI_MODE_0,
    .wordSz = 8,
    .dataLines = 1,
};

/*
 * EXTCOMIN waveform: 60 Hz, 50% duty. The counts are in the board PWM's
 * (prescaled) tick units. On the STM32WB55 Nucleo, LPTIM1 runs at /128 (2 us
 * per tick), so 8333 ticks is ~60 Hz. A board whose PWM ticks differently must
 * scale these to keep EXTCOMIN within the panel's 54-65 Hz range.
 */
static whal_Pwm_ChannelCfg g_ls013VcomCfg = {
    .periodCycles = 8333,
    .pulseCycles  = 4167,
    .pulseCount   = WHAL_PWM_PULSE_COUNT_CONTINUOUS,
    .polarity     = WHAL_PWM_POLARITY_NORMAL,
};

whal_Display g_whalSharpLs013b7dh03 = {
    .driver = &whal_SharpMemory_Display_Driver,
    .cfg = &(whal_SharpMemory_Display_Cfg) {
        .spiDev     = BOARD_SPI_DEV,
        .spiComCfg  = &g_ls013ComCfg,
        .gpioDev    = BOARD_GPIO_DEV,
        .csPin      = SPI_CS_PIN,
        .pwm        = BOARD_PWM_DEV,
        .pwmChannel = 0,
        .vcomCfg    = &g_ls013VcomCfg,
        .width      = 128,
        .height     = 128,
    },
};
