/* wolfHAL_board.c
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

/* Board configuration for the ESP32-C6-DevKitC-1 dev board */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"
#include "wolfHAL_peripheral.h"

volatile uint32_t g_tick = 0;

static uint32_t Board_GetTick(void)
{
    return g_tick;
}

whal_Timeout g_whalTimeout = {
    .timeoutTicks = 1000,
    .GetTick = Board_GetTick,
};

whal_Uart g_whalUart = WHAL_CFG_ESP32C6_UART_DEV;
whal_Timer g_whalTimer = WHAL_CFG_ESP32C6_SYSTIMER_DEV;

static const whal_Esp32c6_Clock_PeriphClk g_periphClks[] = {
    {WHAL_ESP32C6_UART0_GATE},
    {WHAL_ESP32C6_IOMUX_GATE},
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))

void Board_WaitMs(size_t ms)
{
    (void)ms;
}

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Configure the SysClk to use PLL at 160 MHz */
    whal_Esp32c6_Clock_SetSysClk(WHAL_ESP32C6_HP_SYS_CLK_PLL);
    whal_Esp32c6_Clock_ConfigureCpuFreq(WHAL_ESP32C6_CPU_LS_DIV_1,
                                        WHAL_ESP32C6_CPU_HS_DIV_1,
                                        WHAL_ESP32C6_CPU_HS_120M_FORCE_DISABLE);

    /* Enable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Esp32c6_Clock_EnablePeriphClk(&g_periphClks[i]);
        if (err) {
            return err;
        }
    }

    err = whal_Gpio_Init(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = Peripheral_Init();
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error Board_Deinit(void)
{
    whal_Error err;

    err = Peripheral_Deinit();
    if (err) {
        return err;
    }

    err = whal_Gpio_Deinit(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    /* Disable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Esp32c6_Clock_DisablePeriphClk(&g_periphClks[i]);
        if (err) {
            return err;
        }
    }

    return WHAL_SUCCESS;
}
