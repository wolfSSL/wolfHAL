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

/* Example board configuration for the PIC32CZ CA Curiosity Ultra dev board */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"
#include <wolfHAL/platform/microchip/pic32cz.h>
#include "wolfHAL_peripheral.h"

/* SysTick timing (must precede g_whalTimeout below) */
volatile uint32_t g_tick = 0;
volatile uint8_t g_waiting = 0;
volatile uint8_t g_tickOverflow = 0;

uint32_t Board_GetTick(void)
{
    return g_tick;
}

whal_Timeout g_whalTimeout = {
    .timeoutTicks = 1000,
    .GetTick = Board_GetTick,
};

/* SUPC singleton lives in wolfHAL_board.h as `static const`. */

/* Peripheral clocks */
static const whal_Pic32cz_Clock_PeriphClk g_periphClks[] = {
    { /* SERCOM 4 (UART) */
        .gclkPeriphChannel = 25,
        .gclkPeriphSrc = 0, /* GEN 0 */
        .mclkEnableInst = 1,
        .mclkEnableMask = (1UL << 3),
        .mclkEnablePos = 3,
    },
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))


/* UART */
whal_Uart g_whalUart = {
    .base = WHAL_PIC32CZ_SERCOM4_UART_BASE,
    /* .driver: direct API mapping */

    .cfg = &(whal_Pic32cz_Uart_Cfg) {
        .baud = WHAL_PIC32CZ_UART_BAUD(115200, 300000000),
        .txPad = WHAL_PIC32CZ_UART_TXPO_PAD0,
        .rxPad = WHAL_PIC32CZ_UART_RXPO_PAD1,
    },
};

void SysTick_Handler()
{
    uint32_t tickBefore = g_tick++;
    if (g_waiting) {
        if (tickBefore > g_tick)
            g_tickOverflow = 1;
    }
}

void Board_WaitMs(size_t ms)
{
    uint32_t startCount = g_tick;
    g_waiting = 1;
    while (1) {
        uint32_t currentCount = g_tick;
        if (g_tickOverflow) {
            if ((UINT32_MAX - startCount) + currentCount > ms) {
                break;
            }
        } else if (currentCount - startCount > ms) {
            break;
        }
    }

    g_waiting = 0;
    g_tickOverflow = 0;
}

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Enable PLL power supply before clock init */
    err = whal_Pic32cz_Supc_EnableSupply(
            &(whal_Pic32cz_Supc_Supply){WHAL_PIC32CZ_SUPC_PLL});
    if (err) {
        return err;
    }

    /* PLL0: DFLL48 / 12 * 225 / 3 = 300 MHz, then GCLK0 from PLL0 OUT0,
     * then MCLK CPU divider /2 = 150 MHz CPU. */
    err = whal_Pic32cz_Clock_EnablePll(&(whal_Pic32cz_Clock_PllCfg){
        .pllInst = WHAL_PIC32CZ_PLL0,
        .refSel = WHAL_PIC32CZ_REFSEL_DFLL48M,
        .bwSel = WHAL_PIC32CZ_BWSEL_10MHz_TO_20MHz,
        .fbDiv = 225,
        .refDiv = 12,
        .outCfgCount = 1,
        .outCfg = &(whal_Pic32cz_Clock_PllOutCfg){
            .postDivMask = WHAL_PIC32CZ_POSTDIV0_Msk,
            .postDivPos  = WHAL_PIC32CZ_POSTDIV0_Pos,
            .outEnMask   = WHAL_PIC32CZ_OUTEN0_Msk,
            .outEnPos    = WHAL_PIC32CZ_OUTEN0_Pos,
            .postDiv = 3,
        },
    });
    if (err)
        return err;
    err = whal_Pic32cz_Clock_SetMclkDiv(2);
    if (err)
        return err;
    err = whal_Pic32cz_Clock_EnableGclkGen(&(whal_Pic32cz_Clock_GenCfg){
        .gen = 0,
        .genSrc = WHAL_PIC32CZ_GENSRC_PLL0_CLOCKOUT0,
        .genDiv = 1,
    });
    if (err)
        return err;

    /* Enable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Pic32cz_Clock_EnablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    err = whal_Gpio_Init(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Uart_Init(&g_whalUart);
    if (err) {
        return err;
    }

    err = whal_Flash_Init(BOARD_FLASH_DEV);
    if (err) {
        return err;
    }

    err = whal_Timer_Init(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Timer_Start(WHAL_INTERNAL_DEV);
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

    err = whal_Timer_Stop(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Timer_Deinit(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Flash_Deinit(BOARD_FLASH_DEV);
    if (err) {
        return err;
    }

    err = whal_Uart_Deinit(&g_whalUart);
    if (err) {
        return err;
    }

    err = whal_Gpio_Deinit(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    /* Disable peripheral clocks */
    for (size_t i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Pic32cz_Clock_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    /* SUPC outputs are left as-is; no Deinit operation. */

    return WHAL_SUCCESS;
}
