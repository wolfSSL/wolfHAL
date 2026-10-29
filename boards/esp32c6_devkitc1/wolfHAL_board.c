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
whal_Watchdog g_whalTimg0Wdt = WHAL_CFG_ESP32C6_TIMG_WDT_DEV;

static const whal_Esp32c6_Clock_PeriphClk g_periphClks[] = {
    {WHAL_ESP32C6_UART0_GATE},
    {WHAL_ESP32C6_IOMUX_GATE},
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))

void __attribute__((interrupt)) Systimer_Handler(void)
{
    whal_Esp32c6_Systimer_IrqHandler(&g_whalTimer);
    g_tick++;
}

void Board_WaitMs(size_t ms)
{
    uint32_t startCount = g_tick;
    while (g_tick - startCount < ms);
}

whal_Error Board_Init(void)
{
    whal_Error err;

    /* Let bus masters other than the CPU, such as peripheral DMA, reach memory */
    whal_Reg_Write(WHAL_ESP32C6_HP_APM_BASE, WHAL_ESP32C6_APM_FUNC_CTRL_REG, 0);
    whal_Reg_Write(WHAL_ESP32C6_LP_APM0_BASE, WHAL_ESP32C6_APM_FUNC_CTRL_REG, 0);
    whal_Reg_Write(WHAL_ESP32C6_LP_APM_BASE, WHAL_ESP32C6_APM_FUNC_CTRL_REG, 0);

    /* Direct Boot leaves the ROM's boot watchdogs armed */
    err = whal_Watchdog_Deinit(&g_whalTimg0Wdt);
    if (err) {
        return err;
    }

    err = whal_Esp32c6_RtcWdt_Ext_DisableSwd(BOARD_RTC_WDT_DEV);
    if (err) {
        return err;
    }

    err = whal_Watchdog_Deinit(BOARD_RTC_WDT_DEV);
    if (err) {
        return err;
    }

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

    err = whal_Uart_Init(&g_whalUart);
    if (err) {
        return err;
    }

    err = whal_Flash_Init(BOARD_FLASH_DEV);
    if (err) {
        return err;
    }

    err = whal_Irq_Init(WHAL_INTERNAL_DEV);
    if (err) {
        return err;
    }

    err = whal_Timer_Init(&g_whalTimer);
    if (err) {
        return err;
    }

    err = whal_Irq_Enable(WHAL_INTERNAL_DEV, WHAL_ESP32C6_INTSRC_SYSTIMER_TARGET0,
                          &(whal_Esp32c6_Intc_Cfg){
                              .cpuLine = BOARD_SYSTIMER_CPU_INT,
                              .priority = 1,
                          });
    if (err) {
        return err;
    }

    err = whal_Timer_Start(&g_whalTimer);
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

    err = whal_Timer_Stop(&g_whalTimer);
    if (err) {
        return err;
    }

    err = whal_Timer_Deinit(&g_whalTimer);
    if (err) {
        return err;
    }

    err = whal_Irq_Disable(WHAL_INTERNAL_DEV, WHAL_ESP32C6_INTSRC_SYSTIMER_TARGET0);
    if (err) {
        return err;
    }

    err = whal_Irq_Deinit(WHAL_INTERNAL_DEV);
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
        err = whal_Esp32c6_Clock_DisablePeriphClk(&g_periphClks[i]);
        if (err) {
            return err;
        }
    }

    return WHAL_SUCCESS;
}
