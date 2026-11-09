/* test_esp32c6_irq.c
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
#include <wolfHAL/irq/esp32c6_intc.h>
#include "wolfHAL_board.h"
#include "test.h"

#define INTMTX_SOURCE_MAP_REG(src)  (4 * (src))
#define INTMTX_SOURCE_MAP_Msk       0x1FUL

#define PLIC_MXINT_ENABLE_REG       0x0000
#define PLIC_MXINT_PRI_REG(n)       (0x0010 + 4 * (n))
#define PLIC_MXINT_PRI_Msk          0x0FUL

#define TEST_IRQ_SRC                WHAL_ESP32C6_INTSRC_SYSTIMER_TARGET2
#define TEST_IRQ_LINE               5
#define TEST_IRQ_PRIORITY           3

static size_t IntmtxBase(void)
{
    return whal_Esp32c6_Intc_Dev.base;
}

static size_t PlicMxBase(void)
{
    return ((const whal_Esp32c6_Intc_DevCfg *)whal_Esp32c6_Intc_Dev.cfg)
        ->plicMxBase;
}

static size_t SourceMap(size_t src)
{
    return whal_Reg_Read(IntmtxBase(), INTMTX_SOURCE_MAP_REG(src)) &
           INTMTX_SOURCE_MAP_Msk;
}

static whal_Error EnableTestIrq(size_t src, uint8_t cpuLine, uint8_t priority)
{
    return whal_Irq_Enable(WHAL_INTERNAL_DEV, src,
                           &(whal_Esp32c6_Intc_Cfg){
                               .cpuLine = cpuLine,
                               .priority = priority,
                           });
}

static void Test_Intc_TickRouted(void)
{
    WHAL_ASSERT_EQ(SourceMap(WHAL_ESP32C6_INTSRC_SYSTIMER_TARGET0),
                   BOARD_SYSTIMER_CPU_INT);
    WHAL_ASSERT_EQ(whal_Reg_Read(PlicMxBase(), PLIC_MXINT_ENABLE_REG) &
                   (1UL << BOARD_SYSTIMER_CPU_INT),
                   1UL << BOARD_SYSTIMER_CPU_INT);
}

static void Test_Intc_RejectsBadCfg(void)
{
    static const uint8_t clintLines[] = {0, 3, 4, 7};

    WHAL_ASSERT_EQ(whal_Irq_Enable(WHAL_INTERNAL_DEV, TEST_IRQ_SRC, NULL),
                   WHAL_EINVAL);

    for (size_t i = 0; i < sizeof(clintLines); i++) {
        WHAL_ASSERT_EQ(EnableTestIrq(TEST_IRQ_SRC, clintLines[i],
                                     TEST_IRQ_PRIORITY), WHAL_ENOTSUP);
    }
    WHAL_ASSERT_EQ(EnableTestIrq(TEST_IRQ_SRC, 32, TEST_IRQ_PRIORITY),
                   WHAL_ENOTSUP);

    WHAL_ASSERT_EQ(EnableTestIrq(TEST_IRQ_SRC, TEST_IRQ_LINE, 0),
                   WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(EnableTestIrq(TEST_IRQ_SRC, TEST_IRQ_LINE, 16),
                   WHAL_ENOTSUP);

    WHAL_ASSERT_EQ(EnableTestIrq(WHAL_ESP32C6_INTSRC_COUNT, TEST_IRQ_LINE,
                                 TEST_IRQ_PRIORITY), WHAL_ENOTSUP);
    WHAL_ASSERT_EQ(whal_Irq_Disable(WHAL_INTERNAL_DEV,
                                    WHAL_ESP32C6_INTSRC_COUNT), WHAL_ENOTSUP);

    WHAL_ASSERT_EQ(SourceMap(TEST_IRQ_SRC), 0);
}

static void Test_Intc_EnableDisable(void)
{
    whal_Error enableErr;
    whal_Error disableErr;
    size_t mapEnabled;
    size_t priority;
    size_t lineEnabled;
    size_t mapDisabled;

    enableErr = EnableTestIrq(TEST_IRQ_SRC, TEST_IRQ_LINE, TEST_IRQ_PRIORITY);
    mapEnabled = SourceMap(TEST_IRQ_SRC);
    priority = whal_Reg_Read(PlicMxBase(), PLIC_MXINT_PRI_REG(TEST_IRQ_LINE)) &
               PLIC_MXINT_PRI_Msk;
    lineEnabled = whal_Reg_Read(PlicMxBase(), PLIC_MXINT_ENABLE_REG) &
                  (1UL << TEST_IRQ_LINE);

    disableErr = whal_Irq_Disable(WHAL_INTERNAL_DEV, TEST_IRQ_SRC);
    mapDisabled = SourceMap(TEST_IRQ_SRC);

    WHAL_ASSERT_EQ(enableErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(disableErr, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(mapEnabled, TEST_IRQ_LINE);
    WHAL_ASSERT_EQ(priority, TEST_IRQ_PRIORITY);
    WHAL_ASSERT_EQ(lineEnabled, 1UL << TEST_IRQ_LINE);
    WHAL_ASSERT_EQ(mapDisabled, 0);
}

void whal_Test_Irq_Platform(void)
{
    WHAL_TEST_SUITE_START("irq_platform");
    WHAL_TEST(Test_Intc_TickRouted);
    WHAL_TEST(Test_Intc_RejectsBadCfg);
    WHAL_TEST(Test_Intc_EnableDisable);
    WHAL_TEST_SUITE_END();
}
