/* wolfHAL_board.h
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

#ifndef WOLFHAL_BOARD_H
#define WOLFHAL_BOARD_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/platform/esp/esp32c6.h>

extern whal_Uart g_whalUart;
extern whal_Timer g_whalTimer;
extern whal_Watchdog g_whalTimg0Wdt;
extern whal_Timeout g_whalTimeout;
extern whal_Timeout g_whalUsbSerialTimeout;
extern volatile uint32_t g_tick;

#define BOARD_LED_PIN 0
#define BOARD_LED_ON  1
#define BOARD_FLASH_START_ADDR 0x42000000
#define BOARD_FLASH_SIZE       0x00400000
#define BOARD_FLASH_TEST_ADDR  0x423F0000
#define BOARD_FLASH_SECTOR_SZ  0x1000

/* CPU interrupt line carrying the SYSTIMER tick; ivt.c slot 1 must match. */
#define BOARD_SYSTIMER_CPU_INT 1

/* CPU interrupt line carrying the 802.15.4 MAC; ivt.c slot 2 must match. */
#define BOARD_IEEE802154_CPU_INT 2

/* BOARD_*_DEV: how this board reaches each peripheral. */
#define BOARD_GPIO_DEV  WHAL_INTERNAL_DEV
#define BOARD_UART_DEV  (&g_whalUart)
#define BOARD_USB_SERIAL_DEV ((whal_Uart *)&whal_Esp32c6_UsbSerialJtag_Dev)
#define BOARD_TIMER_DEV (&g_whalTimer)
#define BOARD_RTC_WDT_DEV ((whal_Watchdog *)&whal_Esp32c6_RtcWdt_Dev)
#define BOARD_FLASH_DEV   ((whal_Flash *)&whal_Esp32c6_Flash_Dev)
#define BOARD_IEEE802154_DEV ((whal_Ieee802154 *)&whal_Esp32c6_Ieee802154_Dev)

#ifdef BOARD_RADIO
extern uint8_t g_whalEui64[WHAL_IEEE802154_EXT_ADDR_SZ];
#endif

/* GPIO dev initializer: GPIO8 (BOARD_LED_PIN) as a simple push-pull output,
 * GPIO16/GPIO17 as UART0 TX/RX through the IO MUX. */
#define WHAL_CFG_ESP32C6_GPIO_DEV { \
    .base = WHAL_ESP32C6_GPIO_BASE, \
    /* .driver: direct API mapping */ \
    .cfg = (void *)&(const whal_Esp32c6_Gpio_Cfg){ \
        .pinCfg = (const whal_Esp32c6_Gpio_PinCfg[]){ \
            { .pin = 8, \
              .outputEnable = 1, \
              .gpioCfg  = WHAL_ESP32C6_GPIO_PINREG(0, GPIO_INT_DISABLE, 0, 0), \
              .ioMuxCfg = WHAL_ESP32C6_GPIO_IOMUX(IO_MUX_FUNC_GPIO, 0, 0, 2) }, \
            { .pin = 16, \
              .gpioCfg  = WHAL_ESP32C6_GPIO_PINREG(0, GPIO_INT_DISABLE, 0, 0), \
              .ioMuxCfg = WHAL_ESP32C6_GPIO_IOMUX(WHAL_ESP32C6_IOMUX_FUNC_U0TXD, 0, 0, 2) }, \
            { .pin = 17, \
              .gpioCfg  = WHAL_ESP32C6_GPIO_PINREG(0, GPIO_INT_DISABLE, 0, 0), \
              .ioMuxCfg = WHAL_ESP32C6_GPIO_IOMUX(WHAL_ESP32C6_IOMUX_FUNC_U0RXD, 1, \
                                                  IO_MUX_FUN_WPU_Msk, 2) }, \
        }, \
        .pinCount = 3, \
        .outputCfg = (const whal_Esp32c6_Gpio_OutputCfg[]){ \
            { .gpioNum = 8, .cfg = WHAL_ESP32C6_GPIO_OUT(GPIO_SIG_GPIO) }, \
        }, \
        .outputCount = 1, \
    }, \
}

/* UART dev initializer. */
#define WHAL_CFG_ESP32C6_UART_DEV { \
    .base = WHAL_ESP32C6_UART0_BASE, \
    .driver = WHAL_ESP32C6_UART0_DRIVER, \
    .cfg = &(whal_Esp32c6_Uart_Cfg){ \
        .clkDiv = WHAL_ESP32C6_UART_CLKDIV(115200, 40000000), \
        .timeout = &g_whalTimeout, \
    }, \
}

/* SYSTIMER dev initializer: 1 ms periodic tick on UNIT0/COMP0 (16 MHz counter). */
#define WHAL_CFG_ESP32C6_SYSTIMER_DEV { \
    .base = WHAL_ESP32C6_SYSTIMER_BASE, \
    .driver = WHAL_ESP32C6_SYSTIMER_DRIVER, \
    .cfg = &(whal_Esp32c6_Systimer_Cfg){ \
        .mode = WHAL_ESP32C6_SYSTIMER_PERIODIC, \
        .unit = WHAL_ESP32C6_SYSTIMER_UNIT0, \
        .comp = WHAL_ESP32C6_SYSTIMER_COMP0, \
        .compValue = 0, \
        .period = 16000, \
    }, \
}

/* Interrupt controller dev initializer: single-instance device defined in
 * esp32c6_intc.c. */
#define WHAL_CFG_ESP32C6_INTC_DEV { \
    .base = WHAL_ESP32C6_INTMTX_BASE, \
    /* .driver: direct API mapping */ \
    .cfg = (void *)&(const whal_Esp32c6_Intc_DevCfg){ \
        .plicMxBase = WHAL_ESP32C6_PLIC_MX_BASE, \
    }, \
}

/* RTC watchdog dev initializer: single-instance device defined in
 * esp32c6_rtc_wdt.c. stg0Hold is about 1 s of the 136 kHz RTC slow clock. */
#define WHAL_CFG_ESP32C6_RTC_WDT_DEV { \
    .base = WHAL_ESP32C6_RTC_WDT_BASE, \
    .driver = WHAL_ESP32C6_RTC_WDT_DRIVER, \
    .cfg = (void *)&(const whal_Esp32c6_RtcWdt_Cfg){ \
        .stg0Hold = 136000, \
    }, \
}

/* TIMG0 watchdog dev initializer: 40 MHz XTAL / 40000 = 1 kHz, 1 s timeout. */
#define WHAL_CFG_ESP32C6_TIMG_WDT_DEV { \
    .base = WHAL_ESP32C6_TIMG0_BASE, \
    .driver = WHAL_ESP32C6_TIMG_WDT_DRIVER, \
    .cfg = (void *)&(const whal_Esp32c6_TimgWdt_Cfg){ \
        .prescale = 40000, \
        .stg0Hold = 1000, \
    }, \
}

/* Flash dev initializer: single-instance device defined in esp32c6_flash.c. */
#define WHAL_CFG_ESP32C6_FLASH_DEV { \
    .base = WHAL_ESP32C6_SPI1_BASE, \
    .driver = WHAL_ESP32C6_FLASH_DRIVER, \
    .cfg = (void *)&(const whal_Esp32c6_Flash_Cfg){ \
        .startAddr = WHAL_ESP32C6_FLASH_XIP_BASE, \
        .size = 0x00400000, \
    }, \
}

/* 802.15.4 radio dev initializer: single-instance device defined in
 * esp32c6_ieee802154.c. Board_Init reads the EUI-64 from eFuse. */
#define WHAL_CFG_ESP32C6_IEEE802154_DEV { \
    .base = WHAL_ESP32C6_IEEE802154_BASE, \
    .driver = WHAL_ESP32C6_IEEE802154_DRIVER, \
    .cfg = (void *)&(const whal_Esp32c6_Ieee802154_Cfg){ \
        .cpuFreqMhz = 160, \
        .eui64 = g_whalEui64, \
    }, \
}

/* USB Serial/JTAG dev initializer: single-instance device defined in
 * esp32c6_usb_serial_jtag.c. The short timeout keeps output from stalling
 * while no host program has the port open. */
#define WHAL_CFG_ESP32C6_USB_SERIAL_JTAG_DEV { \
    .base = WHAL_ESP32C6_USB_SERIAL_JTAG_BASE, \
    .driver = WHAL_ESP32C6_USB_SERIAL_JTAG_DRIVER, \
    .cfg = (void *)&(const whal_Esp32c6_UsbSerialJtag_Cfg){ \
        .timeout = &g_whalUsbSerialTimeout, \
    }, \
}

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
void Board_WaitMs(size_t ms);

#ifdef BOARD_RADIO
/* Callbacks the 802.15.4 interrupt reports to; NULL ignores events. */
void Board_Ieee802154SetCallbacks(const whal_Ieee802154_Callbacks *cb,
                                  void *ctx);
#endif

#endif /* WOLFHAL_BOARD_H */
