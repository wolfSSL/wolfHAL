/* test_pic32cz_gpio.c
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

#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/gpio/pic32cz_gpio.h>
#include <wolfHAL/bitops.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * PIC32CZ PORT register offsets.
 * Each port group is 0x80 bytes. LED is on port B (index 1), pin 21.
 */
#define PORT_DIR_REG(port) (0x00 + ((port) * 0x80))
#define PORT_OUT_REG(port) (0x10 + ((port) * 0x80))

/* PMUX and PINCFG are byte registers, read here as aligned 32-bit words */
#define PORT_PMUX_REG(port, pin)   (0x30 + (((pin) / 8) * 4) + ((port) * 0x80))
#define PORT_PMUX_Pos(pin)         (((pin) % 8) * 4)
#define PORT_PINCFG_REG(port, pin) (0x40 + ((pin) & ~3) + ((port) * 0x80))
#define PORT_PINCFG_Pos(pin)       (((pin) & 3) * 8)

#define PINCFG_PMUXEN  (1UL << 0)
#define PINCFG_INEN    (1UL << 1)
#define PINCFG_PULLEN  (1UL << 2)

/* LED pin: port B, pin 21 */
#define LED_PORT 1
#define LED_HW_PIN 21

static void Test_Gpio_DirRegister(void)
{
    /* PB21 should be configured as output (bit 21 set in DIR register) */
    uint32_t val = 0;
    whal_Reg32_Get(whal_Pic32cz_Gpio_Dev.base, PORT_DIR_REG(LED_PORT),
                   (1UL << LED_HW_PIN), LED_HW_PIN, &val);
    WHAL_ASSERT_EQ(val, 1);
}

/* Peripheral pins read back with PMUXEN set and their PMUX function; GPIO
 * pins read back with their direction, input enable and pull enable. */
static void Test_Gpio_PinTableRegisters(void)
{
    const whal_Pic32cz_Gpio_Cfg *cfg =
        (const whal_Pic32cz_Gpio_Cfg *)whal_Pic32cz_Gpio_Dev.cfg;
    uintptr_t base = whal_Pic32cz_Gpio_Dev.base;

    for (size_t i = 0; i < cfg->pinCfgCount; i++) {
        const whal_Pic32cz_Gpio_PinCfg *p = &cfg->pinCfg[i];
        size_t pincfg = (whal_Reg32_Read(base,
                                         PORT_PINCFG_REG(p->port, p->pin)) >>
                         PORT_PINCFG_Pos(p->pin)) & 0xFF;
        uint32_t val = 0;

        if (p->pmuxEn) {
            WHAL_ASSERT_EQ(pincfg & PINCFG_PMUXEN, PINCFG_PMUXEN);
            whal_Reg32_Get(base, PORT_PMUX_REG(p->port, p->pin),
                           WHAL_BITMASK(4) << PORT_PMUX_Pos(p->pin),
                           PORT_PMUX_Pos(p->pin), &val);
            WHAL_ASSERT_EQ(val, p->pmux);
            continue;
        }

        WHAL_ASSERT_EQ(pincfg & PINCFG_PMUXEN, 0);
        WHAL_ASSERT_EQ((pincfg & PINCFG_INEN) != 0, p->inEn);
        WHAL_ASSERT_EQ((pincfg & PINCFG_PULLEN) != 0, p->pullEn);

        whal_Reg32_Get(base, PORT_DIR_REG(p->port), 1UL << p->pin, p->pin,
                       &val);
        WHAL_ASSERT_EQ(val, p->dir);
    }
}

static void Test_Gpio_SetHighReg(void)
{
    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 1), WHAL_SUCCESS);

    /* Readback OUT register bit 21 */
    uint32_t val = 0;
    whal_Reg32_Get(whal_Pic32cz_Gpio_Dev.base, PORT_OUT_REG(LED_PORT),
                   (1UL << LED_HW_PIN), LED_HW_PIN, &val);
    WHAL_ASSERT_EQ(val, 1);
}

static void Test_Gpio_SetLowReg(void)
{
    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 0), WHAL_SUCCESS);

    uint32_t val = 0;
    whal_Reg32_Get(whal_Pic32cz_Gpio_Dev.base, PORT_OUT_REG(LED_PORT),
                   (1UL << LED_HW_PIN), LED_HW_PIN, &val);
    WHAL_ASSERT_EQ(val, 0);
}

static void Test_Gpio_SetNonzeroReg(void)
{
    uint32_t val = 0;

    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 2), WHAL_SUCCESS);
    whal_Reg32_Get(whal_Pic32cz_Gpio_Dev.base, PORT_OUT_REG(LED_PORT),
                   (1UL << LED_HW_PIN), LED_HW_PIN, &val);
    WHAL_ASSERT_EQ(val, 1);

    WHAL_ASSERT_EQ(whal_Gpio_Set(BOARD_GPIO_DEV, BOARD_LED_PIN, 0), WHAL_SUCCESS);
}

void whal_Test_Gpio_Platform(void)
{
    WHAL_TEST(Test_Gpio_DirRegister);
    WHAL_TEST(Test_Gpio_PinTableRegisters);
    WHAL_TEST(Test_Gpio_SetHighReg);
    WHAL_TEST(Test_Gpio_SetLowReg);
    WHAL_TEST(Test_Gpio_SetNonzeroReg);
}
