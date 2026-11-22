/* esp32c6.h
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

#ifndef WHAL_ESP32C6_H
#define WHAL_ESP32C6_H

#include <wolfHAL/clock/esp32c6_clock.h>
#include <wolfHAL/gpio/esp32c6_gpio.h>
#include <wolfHAL/flash/esp32c6_flash.h>
#include <wolfHAL/uart/esp32c6_uart.h>
#include <wolfHAL/uart/esp32c6_usb_serial_jtag.h>
#include <wolfHAL/timer/esp32c6_systimer.h>
#include <wolfHAL/irq/esp32c6_intc.h>
#include <wolfHAL/watchdog/esp32c6_rtc_wdt.h>
#include <wolfHAL/watchdog/esp32c6_timg_wdt.h>
#include <wolfHAL/ieee802154/esp32c6_ieee802154.h>

/*
 * @file esp32c6.h
 * @brief Convenience initializers for ESP32-C6 device instances.
 *
 * Peripheral base addresses from the ESP32-C6 TRM (System and Memory, peripheral
 * address table). Peripherals are mapped from 0x6000_0000.
 *   UART0:    0x6000_0000
 *   UART1:    0x6000_1000
 *   SPI1:     0x6000_3000 (memory SPI, not in the TRM address map)
 *   TIMG0:    0x6000_8000
 *   TIMG1:    0x6000_9000
 *   SYSTIMER: 0x6000_A000
 *   USB_SERIAL_JTAG: 0x6000_F000
 *   INTMTX:   0x6001_0000
 *   IO MUX:   0x6009_0000
 *   GPIO:     0x6009_1000
 *   HP_APM:   0x6009_9000
 *   LP_APM0:  0x6009_9800
 *   EFUSE:    0x600B_0800
 *   RTC_WDT:  0x600B_1C00
 *   LP_APM:   0x600B_3800
 *   PLIC MX:  0x2000_1000
 *   802.15.4: 0x600A_3000 (not in the TRM)
 */

/* --- Device macros --- */

#define WHAL_ESP32C6_UART0_BASE   0x60000000
#define WHAL_ESP32C6_UART0_DRIVER &whal_Esp32c6_Uart_Driver

#define WHAL_ESP32C6_UART1_BASE   0x60001000
#define WHAL_ESP32C6_UART1_DRIVER &whal_Esp32c6_Uart_Driver

#define WHAL_ESP32C6_SPI1_BASE    0x60003000
#define WHAL_ESP32C6_FLASH_DRIVER &whal_Esp32c6_Flash_Driver

/* XIP window that the cache maps flash offset 0 into. */
#define WHAL_ESP32C6_FLASH_XIP_BASE 0x42000000

#define WHAL_ESP32C6_TIMG0_BASE      0x60008000
#define WHAL_ESP32C6_TIMG1_BASE      0x60009000
#define WHAL_ESP32C6_TIMG_WDT_DRIVER &whal_Esp32c6_TimgWdt_Driver

#define WHAL_ESP32C6_SYSTIMER_BASE   0x6000A000
#define WHAL_ESP32C6_SYSTIMER_DRIVER &whal_Esp32c6_Systimer_Driver

#define WHAL_ESP32C6_USB_SERIAL_JTAG_BASE   0x6000F000
#define WHAL_ESP32C6_USB_SERIAL_JTAG_DRIVER &whal_Esp32c6_UsbSerialJtag_Driver

#define WHAL_ESP32C6_INTMTX_BASE   0x60010000
#define WHAL_ESP32C6_PLIC_MX_BASE  0x20001000
#define WHAL_ESP32C6_INTC_DRIVER   &whal_Esp32c6_Intc_Driver

#define WHAL_ESP32C6_IOMUX_BASE 0x60090000

/* IO MUX function routing UART0 directly on GPIO16 (TX) and GPIO17 (RX). */
#define WHAL_ESP32C6_IOMUX_FUNC_U0TXD 0
#define WHAL_ESP32C6_IOMUX_FUNC_U0RXD 0

#define WHAL_ESP32C6_GPIO_BASE   0x60091000
#define WHAL_ESP32C6_GPIO_DRIVER &whal_Esp32c6_Gpio_Driver

#define WHAL_ESP32C6_RTC_WDT_BASE   0x600B1C00
#define WHAL_ESP32C6_RTC_WDT_DRIVER &whal_Esp32c6_RtcWdt_Driver

#define WHAL_ESP32C6_IEEE802154_BASE   0x600A3000
#define WHAL_ESP32C6_IEEE802154_DRIVER &whal_Esp32c6_Ieee802154_Driver

/* Access path filters (TRM chapter 16). Enabled at reset, they deny every
 * bus master other than the HP CPU, including peripheral DMA. */
#define WHAL_ESP32C6_HP_APM_BASE             0x60099000
#define WHAL_ESP32C6_LP_APM0_BASE            0x60099800
#define WHAL_ESP32C6_LP_APM_BASE             0x600B3800
#define WHAL_ESP32C6_APM_FUNC_CTRL_REG       0x00C4

/* eFuse read-back of the factory MAC (TRM Registers 6.18, 6.19). SYS_1 holds
 * MAC bits 47:32 in [15:0] and MAC_EXT in [31:16]. */
#define WHAL_ESP32C6_EFUSE_BASE              0x600B0800
#define WHAL_ESP32C6_EFUSE_RD_MAC_SYS_0_REG  0x0044
#define WHAL_ESP32C6_EFUSE_RD_MAC_SYS_1_REG  0x0048

/* --- Interrupt matrix sources (TRM Table 10.3-1) --- */

/* Listed as reserved in the TRM. */
#define WHAL_ESP32C6_INTSRC_ZB_MAC            12

#define WHAL_ESP32C6_INTSRC_UART0             43
#define WHAL_ESP32C6_INTSRC_SYSTIMER_TARGET0  57
#define WHAL_ESP32C6_INTSRC_SYSTIMER_TARGET1  58
#define WHAL_ESP32C6_INTSRC_SYSTIMER_TARGET2  59

/* --- Peripheral clock gates (PCR, TRM chapter 8) --- */

#define WHAL_ESP32C6_UART0_GATE     \
    .confReg = 0x0000,              \
    .clkEnMsk = (1UL << 0),         \
    .rstEnMsk = (1UL << 1),         \
    .funcClkReg = 0x0004,           \
    .funcClkEnMsk = (1UL << 22)

#define WHAL_ESP32C6_IOMUX_GATE     \
    .confReg = 0x00E8,              \
    .clkEnMsk = (1UL << 0),         \
    .rstEnMsk = (1UL << 1),         \
    .funcClkReg = 0x00EC,           \
    .funcClkEnMsk = (1UL << 22)

/* --- Modem clocks (MODEM_SYSCON / MODEM_LPCON, not in the TRM) --- */

/* 802.15.4 MAC with the ETM, modem security and BLE timer clocks it uses. */
#define WHAL_ESP32C6_ZB_MAC_MODEM_CLK           \
    .base = WHAL_ESP32C6_MODEM_SYSCON_BASE,     \
    .clkReg = 0x0004,                           \
    .clkEnMsk = (WHAL_BITMASK(9) << 22),        \
    .rstMsk = (1UL << 22) | (WHAL_BITMASK(4) << 24) | \
              (1UL << 29) | (1UL << 30)

/* Bluetooth/802.15.4 baseband, plus the Wi-Fi baseband clocks the PHY
 * calibration uses. */
#define WHAL_ESP32C6_BB_MODEM_CLK               \
    .base = WHAL_ESP32C6_MODEM_SYSCON_BASE,     \
    .clkReg = 0x0014,                           \
    .clkEnMsk = WHAL_BITMASK(9) | (1UL << 17) | (1UL << 18), \
    .rstMsk = (1UL << 8) | (WHAL_BITMASK(4) << 15)

/* RF front end. */
#define WHAL_ESP32C6_FE_MODEM_CLK               \
    .base = WHAL_ESP32C6_MODEM_SYSCON_BASE,     \
    .clkReg = 0x0014,                           \
    .clkEnMsk = (WHAL_BITMASK(6) << 11),        \
    .rstMsk = (1UL << 14)

/* Radio coexistence arbiter. */
#define WHAL_ESP32C6_COEX_MODEM_CLK             \
    .base = WHAL_ESP32C6_MODEM_LPCON_BASE,      \
    .clkReg = 0x0018,                           \
    .clkEnMsk = (1UL << 1),                     \
    .rstMsk = 0

/* Modem power timer the PHY library times its waits on. */
#define WHAL_ESP32C6_WIFIPWR_MODEM_CLK          \
    .base = WHAL_ESP32C6_MODEM_LPCON_BASE,      \
    .clkReg = 0x0018,                           \
    .clkEnMsk = (1UL << 0),                     \
    .rstMsk = 0

/* Analog I2C master the PHY uses to reach the RF registers, clocked from
 * 160 MHz. */
#define WHAL_ESP32C6_I2C_MST_MODEM_CLK          \
    .base = WHAL_ESP32C6_MODEM_LPCON_BASE,      \
    .clkReg = 0x0018,                           \
    .clkEnMsk = (1UL << 2),                     \
    .rstMsk = 0

#define WHAL_ESP32C6_I2C_MST_160M_MODEM_CLK     \
    .base = WHAL_ESP32C6_MODEM_LPCON_BASE,      \
    .clkReg = 0x0010,                           \
    .clkEnMsk = (1UL << 0),                     \
    .rstMsk = 0

/* Clock gating maps: each modem clock domain runs only in the PMU clock
 * gating codes whose bit is set. The active code depends on how the PMU was
 * last configured (the ROM leaves code 0, ESP-IDF uses others), so these open
 * every domain for every code. */
#define WHAL_ESP32C6_SYSCON_ICG_MODEM_CLK       \
    .base = WHAL_ESP32C6_MODEM_SYSCON_BASE,     \
    .clkReg = 0x000C,                           \
    .clkEnMsk = 0xFFFFFF00,                     \
    .rstMsk = 0

#define WHAL_ESP32C6_LPCON_ICG_MODEM_CLK        \
    .base = WHAL_ESP32C6_MODEM_LPCON_BASE,      \
    .clkReg = 0x0020,                           \
    .clkEnMsk = 0xFFFF0000,                     \
    .rstMsk = 0

#endif /* WHAL_ESP32C6_H */
