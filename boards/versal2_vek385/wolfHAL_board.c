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

/* Example board configuration for the AMD VEK385 (Versal Gen 2) */

#include <stdint.h>
#include <stddef.h>
#include "wolfHAL_board.h"

/* Arm generic timer (CNTPCT_EL0), as a millisecond tick. */
static uint64_t Board_TimerFreq(void)
{
    uint64_t freq;

    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
}

static uint64_t Board_TimerCount(void)
{
    uint64_t count;

    __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(count) : : "memory");
    return count;
}

#ifdef WHAL_CFG_64BIT_TICK
uint64_t Board_GetTick(void)
{
    return Board_TimerCount() / (Board_TimerFreq() / 1000);
}
#else
uint32_t Board_GetTick(void)
{
    return (uint32_t)(Board_TimerCount() / (Board_TimerFreq() / 1000));
}
#endif

whal_Timeout g_whalTimeout = {
    .timeoutTicks = 1000, /* 1s timeout */
    .GetTick = Board_GetTick,
};

/* UART */
whal_Uart g_whalUart = {
    .base = WHAL_VERSAL2_UART1_BASE,
    .driver = WHAL_VERSAL2_UART_DRIVER,
    .cfg = &(whal_Pl011_Uart_Cfg) {
        .ibrd = WHAL_PL011_UART_IBRD(BOARD_UART_CLK_HZ, BOARD_UART_BAUD),
        .fbrd = WHAL_PL011_UART_FBRD(BOARD_UART_CLK_HZ, BOARD_UART_BAUD),
        .lcrH = WHAL_PL011_UART_LCRH_8N1_FIFO,
        .timeout = &g_whalTimeout,
    },
};

void Board_WaitMs(size_t ms)
{
    uint64_t start = Board_TimerCount();
    uint64_t ticks = (Board_TimerFreq() / 1000) * ms;

    while ((Board_TimerCount() - start) < ticks) {
    }
}

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Clocks and pins are set up by the boot firmware (PLM). */
    err = whal_Uart_Init(&g_whalUart);
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}

whal_Error Board_Deinit(void)
{
    whal_Error err;

    err = whal_Uart_Deinit(&g_whalUart);
    if (err) {
        return err;
    }

    return WHAL_SUCCESS;
}
