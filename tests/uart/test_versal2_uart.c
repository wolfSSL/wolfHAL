/* test_versal2_uart.c
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
#include <wolfHAL/reg.h>
#include "wolfHAL_board.h"
#include "test.h"

/*
 * PL011 tests on UART0, in internal loopback (CR.LBE): what UART0 sends
 * comes back on its own RX, so no cable is needed, on QEMU or the board.
 * UART1 stays the console.
 */

/* Register offsets and bits (AM028 UART module, Arm DDI 0183G 3.3). */
#define UART_DR_REG   0x00
#define UART_FR_REG   0x18
#define UART_IBRD_REG 0x24
#define UART_FBRD_REG 0x28
#define UART_LCRH_REG 0x2C
#define UART_CR_REG   0x30

#define UART_FR_RXFE  (1UL << 4)
#define UART_LCRH_BRK (1UL << 0)
#define UART_CR_EN    ((1UL << 0) | (1UL << 8) | (1UL << 9))
#define UART_CR_LBE   (1UL << 7)

static whal_Pl011_Uart_Cfg g_loopCfg = {
    .ibrd = WHAL_PL011_UART_IBRD(BOARD_UART_CLK_HZ, BOARD_UART_BAUD),
    .fbrd = WHAL_PL011_UART_FBRD(BOARD_UART_CLK_HZ, BOARD_UART_BAUD),
    .lcrH = WHAL_PL011_UART_LCRH_8N1_FIFO,
    .timeout = &g_whalTimeout,
};

static whal_Uart g_loopUart = {
    .base = WHAL_VERSAL2_UART0_BASE,
    .driver = WHAL_VERSAL2_UART_DRIVER,
    .cfg = &g_loopCfg,
};

#define LOOP_BASE WHAL_VERSAL2_UART0_BASE

/* Send returns only when all bytes are out, so the RX FIFO must hold a
 * whole chunk: 32 deep on the PL011, 16 in QEMU's model. */
#define LOOP_CHUNK 8

/* Init, then turn on loopback. Init itself writes CR without LBE. */
static whal_Error Loop_Start(void)
{
    whal_Error err;

    err = whal_Uart_Init(&g_loopUart);
    if (err) {
        return err;
    }
    whal_Reg32_Update(LOOP_BASE, UART_CR_REG, UART_CR_LBE, UART_CR_LBE);

    return WHAL_SUCCESS;
}

static void Test_Versal2Uart_InitRegs(void)
{
    WHAL_ASSERT_EQ(whal_Uart_Init(&g_loopUart), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_IBRD_REG), g_loopCfg.ibrd);
    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_FBRD_REG), g_loopCfg.fbrd);
    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_LCRH_REG),
                   WHAL_PL011_UART_LCRH_8N1_FIFO);
    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_CR_REG), UART_CR_EN);
}

static void Test_Versal2Uart_Loopback(void)
{
    static const uint8_t tx[LOOP_CHUNK * 4] = "wolfHAL PL011 UART0 loopback";
    static uint8_t rx[sizeof(tx)];
    size_t i;

    WHAL_ASSERT_EQ(Loop_Start(), WHAL_SUCCESS);
    for (i = 0; i < sizeof(tx); i += LOOP_CHUNK) {
        WHAL_ASSERT_EQ(whal_Uart_Send(&g_loopUart, &tx[i], LOOP_CHUNK),
                       WHAL_SUCCESS);
        WHAL_ASSERT_EQ(whal_Uart_Recv(&g_loopUart, &rx[i], LOOP_CHUNK),
                       WHAL_SUCCESS);
    }
    WHAL_ASSERT_MEM_EQ(rx, tx, sizeof(tx));

    /* Every byte was taken off the RX FIFO. */
    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_FR_REG) & UART_FR_RXFE,
                   UART_FR_RXFE);
}

static void Test_Versal2Uart_RecvTimeout(void)
{
#ifdef WHAL_CFG_NO_TIMEOUT
    WHAL_SKIP();
#else
    uint8_t rx = 0;

    WHAL_ASSERT_EQ(Loop_Start(), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Uart_Recv(&g_loopUart, &rx, 1), WHAL_ETIMEOUT);
#endif
}

/* A break on the line arrives as a byte with the BE flag set (Arm DDI
 * 0183G UARTDR); Recv drops it and reports a hardware error. */
static void Test_Versal2Uart_RecvBreak(void)
{
    const uint8_t tx = 0x5A;
    uint8_t rx = 0;

    WHAL_ASSERT_EQ(Loop_Start(), WHAL_SUCCESS);
    whal_Reg32_Update(LOOP_BASE, UART_LCRH_REG, UART_LCRH_BRK, UART_LCRH_BRK);
    Board_WaitMs(2);
    whal_Reg32_Update(LOOP_BASE, UART_LCRH_REG, UART_LCRH_BRK, 0);

    WHAL_ASSERT_EQ(whal_Uart_Recv(&g_loopUart, &rx, 1), WHAL_EHARDWARE);

    /* The UART still works after the error. */
    WHAL_ASSERT_EQ(whal_Uart_Send(&g_loopUart, &tx, 1), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Uart_Recv(&g_loopUart, &rx, 1), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(rx, tx);
}

static void Test_Versal2Uart_BadDivisor(void)
{
    whal_Pl011_Uart_Cfg cfg = g_loopCfg;
    whal_Uart uart = g_loopUart;

    uart.cfg = &cfg;
    WHAL_ASSERT_EQ(whal_Uart_Init(&g_loopUart), WHAL_SUCCESS);

    cfg.ibrd = 0;
    WHAL_ASSERT_EQ(whal_Uart_Init(&uart), WHAL_EINVAL);
    cfg.ibrd = 0xFFFF;
    cfg.fbrd = 1;
    WHAL_ASSERT_EQ(whal_Uart_Init(&uart), WHAL_EINVAL);

    /* A refused Init leaves the UART as it was. */
    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_IBRD_REG), g_loopCfg.ibrd);
    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_CR_REG), UART_CR_EN);
}

static void Test_Versal2Uart_Deinit(void)
{
    WHAL_ASSERT_EQ(whal_Uart_Init(&g_loopUart), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Uart_Deinit(&g_loopUart), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_CR_REG), 0);
    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_IBRD_REG), 0);
    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_FBRD_REG), 0);
    WHAL_ASSERT_EQ(whal_Reg32_Read(LOOP_BASE, UART_LCRH_REG), 0);
}

void whal_Test_Uart_Platform(void)
{
    WHAL_TEST_SUITE_START("uart_platform");
    WHAL_TEST(Test_Versal2Uart_InitRegs);
    WHAL_TEST(Test_Versal2Uart_Loopback);
    WHAL_TEST(Test_Versal2Uart_RecvTimeout);
    WHAL_TEST(Test_Versal2Uart_RecvBreak);
    WHAL_TEST(Test_Versal2Uart_BadDivisor);
    WHAL_TEST(Test_Versal2Uart_Deinit);
    WHAL_TEST_SUITE_END();
}
