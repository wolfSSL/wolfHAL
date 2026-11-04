/* test_pl011_uart.c
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

#include <stdint.h>
#include <string.h>
#include <wolfHAL/uart/uart.h>
#include <wolfHAL/uart/pl011_uart.h>
#include <wolfHAL/platform/amd/versal2.h>
#include "../test.h"

/* Runs the driver on registers in memory. There are no FIFOs, so each test
 * sets the FR bits it needs. */

/* Register word index (offset / 4) */
#define DR    (0x00 / 4)
#define ECR   (0x04 / 4)
#define FR    (0x18 / 4)
#define IBRD  (0x24 / 4)
#define FBRD  (0x28 / 4)
#define LCRH  (0x2C / 4)
#define CR    (0x30 / 4)
#define NREGS (0x48 / 4)

#define FR_BUSY (1UL << 3)
#define FR_RXFE (1UL << 4)
#define FR_TXFF (1UL << 5)
#define FR_TXFE (1UL << 7)

#define CR_UARTEN (1UL << 0)
#define CR_TXE    (1UL << 8)
#define CR_RXE    (1UL << 9)
#define CR_CTSEN  (1UL << 15)

#define SENTINEL 0xA5A5A5A5UL

static uint32_t g_regs[NREGS];

#ifndef WHAL_CFG_NO_TIMEOUT
/* Fake tick that moves on every call, so a stuck poll runs out. */
#ifdef WHAL_CFG_64BIT_TICK
static uint64_t g_fakeTick;

static uint64_t FakeTick(void)
{
    return g_fakeTick++;
}
#else
static uint32_t g_fakeTick;

static uint32_t FakeTick(void)
{
    return g_fakeTick++;
}
#endif

static whal_Timeout g_timeout = {
    .timeoutTicks = 5,
    .GetTick = FakeTick,
};

static whal_Timeout *timeout(void)
{
    return &g_timeout;
}
#else
static whal_Timeout *timeout(void)
{
    return NULL;
}
#endif

static whal_Pl011_Uart_Cfg g_cfg;

/* Reset the register block and the config to 100 MHz, 115200 8N1. */
static void Reset(void)
{
    size_t i;

    for (i = 0; i < NREGS; i++) {
        g_regs[i] = SENTINEL;
    }
    /* Idle: TX FIFO empty, RX FIFO empty, not busy */
    g_regs[FR] = FR_TXFE | FR_RXFE;
    g_regs[CR] = 0;

    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.ibrd = WHAL_PL011_UART_IBRD(100000000, 115200);
    g_cfg.fbrd = WHAL_PL011_UART_FBRD(100000000, 115200);
    g_cfg.lcrH = WHAL_PL011_UART_LCRH_8N1_FIFO;
    g_cfg.timeout = timeout();
}

#define MAKE_DEV(name)                             \
    whal_Uart name = {                             \
        .base = (size_t)g_regs,                    \
        .driver = WHAL_VERSAL2_UART_DRIVER,        \
        .cfg = &g_cfg,                             \
    }

static void Test_Pl011_Platform(void)
{
    WHAL_ASSERT_EQ(WHAL_VERSAL2_UART0_BASE, 0xF1920000);
    WHAL_ASSERT_EQ(WHAL_VERSAL2_UART1_BASE, 0xF1930000);
}

static void Test_Pl011_Divisors(void)
{
    /* 100 MHz: 3472.2 / 64 -> 54 + 16/64 */
    WHAL_ASSERT_EQ(WHAL_PL011_UART_IBRD(100000000, 115200), 54);
    WHAL_ASSERT_EQ(WHAL_PL011_UART_FBRD(100000000, 115200), 16);
    /* 125 MHz: 4340.3 -> 67 + 52/64 */
    WHAL_ASSERT_EQ(WHAL_PL011_UART_IBRD(125000000, 115200), 67);
    WHAL_ASSERT_EQ(WHAL_PL011_UART_FBRD(125000000, 115200), 52);
    /* 150 MHz: 5208.3 -> 81 + 24/64 */
    WHAL_ASSERT_EQ(WHAL_PL011_UART_IBRD(150000000, 115200), 81);
    WHAL_ASSERT_EQ(WHAL_PL011_UART_FBRD(150000000, 115200), 24);
    /* AM026 ch. 63 p670 example: 4 MHz, 230400 -> DIVINT 1, DIVFRAC 5 */
    WHAL_ASSERT_EQ(WHAL_PL011_UART_IBRD(4000000, 230400), 1);
    WHAL_ASSERT_EQ(WHAL_PL011_UART_FBRD(4000000, 230400), 5);
}

static void Test_Pl011_DivisorCarry(void)
{
    /* 319 * 4 / 10 = 127.6. The fraction rounds up to 64, so it carries:
     * 2 + 0/64, not 1 + 64/64 or a cut-off 1 + 63/64. */
    WHAL_ASSERT_EQ(WHAL_PL011_UART_DIV64(319, 10), 128);
    WHAL_ASSERT_EQ(WHAL_PL011_UART_IBRD(319, 10), 2);
    WHAL_ASSERT_EQ(WHAL_PL011_UART_FBRD(319, 10), 0);
    /* 127.4 rounds down */
    WHAL_ASSERT_EQ(WHAL_PL011_UART_DIV64(3185, 100), 127);
}

static void Test_Pl011_LcrhDefault(void)
{
    WHAL_ASSERT_EQ(WHAL_PL011_UART_LCRH_8N1_FIFO, 0x70);
}

static void Test_Pl011_Init(void)
{
    whal_Error err;

    Reset();
    MAKE_DEV(dev);
    g_regs[CR] = CR_UARTEN | CR_CTSEN;

    err = whal_Uart_Init(&dev);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[IBRD], 54);
    WHAL_ASSERT_EQ(g_regs[FBRD], 16);
    WHAL_ASSERT_EQ(g_regs[LCRH], 0x70);
    /* Full CR write: flow control and other bits cleared */
    WHAL_ASSERT_EQ(g_regs[CR], CR_UARTEN | CR_TXE | CR_RXE);
    /* Registers the driver does not own are left alone */
    WHAL_ASSERT_EQ(g_regs[DR], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[ECR], SENTINEL);
}

static void Test_Pl011_InitLcrhMasked(void)
{
    whal_Error err;

    Reset();
    MAKE_DEV(dev);
    g_cfg.lcrH = 0xFFFFFF00UL | WHAL_PL011_UART_LCRH_8N1_FIFO |
                 WHAL_PL011_UART_LCRH_PEN;

    err = whal_Uart_Init(&dev);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[LCRH], 0x72);
}

static void Test_Pl011_InitReprograms(void)
{
    whal_Error err;

    /* An earlier boot stage left the UART on with another setup: Init
     * sets it up again from the configuration. */
    Reset();
    MAKE_DEV(dev);
    g_regs[CR] = CR_UARTEN | CR_CTSEN | CR_TXE;
    g_regs[IBRD] = 7;
    g_regs[FBRD] = 3;
    g_regs[LCRH] = 0x60;

    err = whal_Uart_Init(&dev);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[IBRD], 54);
    WHAL_ASSERT_EQ(g_regs[FBRD], 16);
    WHAL_ASSERT_EQ(g_regs[LCRH], 0x70);
    WHAL_ASSERT_EQ(g_regs[CR], CR_UARTEN | CR_TXE | CR_RXE);
}

static void Test_Pl011_InitInvalidDivisors(void)
{
    whal_Error err;

    Reset();
    MAKE_DEV(dev);
    g_regs[CR] = CR_UARTEN;

    g_cfg.ibrd = 0;
    err = whal_Uart_Init(&dev);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);

    g_cfg.ibrd = 0x10000;
    err = whal_Uart_Init(&dev);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);

    g_cfg.ibrd = 0xFFFF;
    g_cfg.fbrd = 64;
    err = whal_Uart_Init(&dev);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);

    /* Nothing was touched */
    WHAL_ASSERT_EQ(g_regs[CR], CR_UARTEN);
    WHAL_ASSERT_EQ(g_regs[IBRD], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[FBRD], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[LCRH], SENTINEL);

    /* The top of both ranges is fine */
    g_cfg.fbrd = 63;
    err = whal_Uart_Init(&dev);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[IBRD], 0xFFFF);
    WHAL_ASSERT_EQ(g_regs[FBRD], 63);
}

static void Test_Pl011_Deinit(void)
{
    whal_Error err;

    Reset();
    MAKE_DEV(dev);
    err = whal_Uart_Init(&dev);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);

    err = whal_Uart_Deinit(&dev);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[CR], 0);
    WHAL_ASSERT_EQ(g_regs[IBRD], 0);
    WHAL_ASSERT_EQ(g_regs[FBRD], 0);
    WHAL_ASSERT_EQ(g_regs[LCRH], 0);
}

static void Test_Pl011_Send(void)
{
    whal_Error err;
    const uint8_t a = 'A';
    const uint8_t ab[2] = { 'A', 'B' };
    const uint8_t nl = '\n';

    Reset();
    MAKE_DEV(dev);
    g_regs[FR] = FR_TXFE;

    err = whal_Uart_Send(&dev, &a, 1);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[DR], 'A');

    /* The array keeps only the last byte written */
    err = whal_Uart_Send(&dev, ab, sizeof(ab));
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[DR], 'B');

    /* No '\n' -> "\r\n" in the driver */
    err = whal_Uart_Send(&dev, &nl, 1);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[DR], '\n');

    /* A 32-bit write: ECR next to DR is not touched */
    WHAL_ASSERT_EQ(g_regs[ECR], SENTINEL);

    /* Zero bytes is fine */
    g_regs[DR] = 0;
    err = whal_Uart_Send(&dev, &a, 0);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(g_regs[DR], 0);
}

static void Test_Pl011_Recv(void)
{
    whal_Error err;
    uint8_t buf[2] = { 0, 0 };

    Reset();
    MAKE_DEV(dev);
    g_regs[FR] = FR_TXFE;
    g_regs[DR] = 0x5A;

    err = whal_Uart_Recv(&dev, buf, 1);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(buf[0], 0x5A);

    g_regs[DR] = 0x33;
    err = whal_Uart_Recv(&dev, buf, 2);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    WHAL_ASSERT_EQ(buf[0], 0x33);
    WHAL_ASSERT_EQ(buf[1], 0x33);
    WHAL_ASSERT_EQ(g_regs[ECR], SENTINEL);
}

static void Test_Pl011_RecvError(void)
{
    whal_Error err;
    uint8_t buf[1];
    size_t bit;

    Reset();
    MAKE_DEV(dev);
    g_regs[FR] = FR_TXFE;

    /* FE, PE, BE, OE are DR bits 8..11 */
    for (bit = 8; bit <= 11; bit++) {
        buf[0] = 0;
        g_regs[DR] = (1UL << bit) | 0x5A;
        g_regs[ECR] = SENTINEL;

        err = whal_Uart_Recv(&dev, buf, 1);
        WHAL_ASSERT_EQ(err, WHAL_EHARDWARE);
        /* Bad byte dropped, error flags cleared through ECR */
        WHAL_ASSERT_EQ(buf[0], 0);
        WHAL_ASSERT_EQ(g_regs[ECR], 0);
    }
}

static void Test_Pl011_NullArgs(void)
{
    whal_Error err;
    uint8_t b = 0;
    whal_Uart noCfg = {
        .base = (size_t)g_regs,
        .driver = &whal_Pl011_Uart_Driver,
        .cfg = NULL,
    };

    Reset();
    MAKE_DEV(dev);

    err = whal_Pl011_Uart_Init(NULL);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    err = whal_Pl011_Uart_Init(&noCfg);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    err = whal_Pl011_Uart_Deinit(NULL);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    err = whal_Pl011_Uart_Deinit(&noCfg);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    err = whal_Pl011_Uart_Send(NULL, &b, 1);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    err = whal_Pl011_Uart_Send(&noCfg, &b, 1);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    err = whal_Pl011_Uart_Send(&dev, NULL, 1);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    err = whal_Pl011_Uart_Recv(NULL, &b, 1);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    err = whal_Pl011_Uart_Recv(&noCfg, &b, 1);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    err = whal_Pl011_Uart_Recv(&dev, NULL, 1);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);

    /* No register was touched */
    WHAL_ASSERT_EQ(g_regs[CR], 0);
    WHAL_ASSERT_EQ(g_regs[DR], SENTINEL);
}

static void Test_Pl011_Async(void)
{
    whal_Error err;
    uint8_t b = 0;

    Reset();
    MAKE_DEV(dev);

    err = whal_Uart_SendAsync(&dev, &b, 1);
    WHAL_ASSERT_EQ(err, WHAL_ENOTSUP);
    err = whal_Uart_RecvAsync(&dev, &b, 1);
    WHAL_ASSERT_EQ(err, WHAL_ENOTSUP);
    err = whal_Pl011_Uart_Driver.SendAsync(NULL, &b, 1);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
    err = whal_Pl011_Uart_Driver.RecvAsync(&dev, NULL, 1);
    WHAL_ASSERT_EQ(err, WHAL_EINVAL);
}

#ifndef WHAL_CFG_NO_TIMEOUT
static void Test_Pl011_InitBusyTimeout(void)
{
    whal_Error err;

    Reset();
    MAKE_DEV(dev);
    g_regs[CR] = CR_UARTEN;
    g_regs[FR] = FR_BUSY;

    err = whal_Uart_Init(&dev);
    WHAL_ASSERT_EQ(err, WHAL_ETIMEOUT);
    /* Disabled first, divisors not written while busy */
    WHAL_ASSERT_EQ(g_regs[CR], 0);
    WHAL_ASSERT_EQ(g_regs[IBRD], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[FBRD], SENTINEL);
    WHAL_ASSERT_EQ(g_regs[LCRH], SENTINEL);
}

static void Test_Pl011_DeinitBusyTimeout(void)
{
    whal_Error err;

    Reset();
    MAKE_DEV(dev);
    err = whal_Uart_Init(&dev);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);
    g_regs[FR] = FR_TXFE | FR_BUSY;

    err = whal_Uart_Deinit(&dev);
    WHAL_ASSERT_EQ(err, WHAL_ETIMEOUT);
    /* Disabled first, setup not cleared while the last byte goes out */
    WHAL_ASSERT_EQ(g_regs[CR], 0);
    WHAL_ASSERT_EQ(g_regs[IBRD], g_cfg.ibrd);
    WHAL_ASSERT_EQ(g_regs[FBRD], g_cfg.fbrd);
    WHAL_ASSERT_EQ(g_regs[LCRH], g_cfg.lcrH);
}

static void Test_Pl011_SendTxffTimeout(void)
{
    whal_Error err;
    const uint8_t a = 'A';

    Reset();
    MAKE_DEV(dev);
    g_regs[DR] = 0;
    g_regs[FR] = FR_TXFF;

    err = whal_Uart_Send(&dev, &a, 1);
    WHAL_ASSERT_EQ(err, WHAL_ETIMEOUT);
    /* FIFO full: the byte was not written */
    WHAL_ASSERT_EQ(g_regs[DR], 0);
}

static void Test_Pl011_SendDrainTimeout(void)
{
    whal_Error err;
    const uint8_t a = 'A';

    /* TXFE never sets */
    Reset();
    MAKE_DEV(dev);
    g_regs[FR] = 0;
    err = whal_Uart_Send(&dev, &a, 1);
    WHAL_ASSERT_EQ(err, WHAL_ETIMEOUT);
    WHAL_ASSERT_EQ(g_regs[DR], 'A');

    /* BUSY never clears */
    g_regs[FR] = FR_TXFE | FR_BUSY;
    err = whal_Uart_Send(&dev, &a, 1);
    WHAL_ASSERT_EQ(err, WHAL_ETIMEOUT);
}

static void Test_Pl011_RecvTimeout(void)
{
    whal_Error err;
    uint8_t buf[1] = { 0 };

    Reset();
    MAKE_DEV(dev);
    g_regs[FR] = FR_RXFE;
    g_regs[DR] = 0x5A;

    err = whal_Uart_Recv(&dev, buf, 1);
    WHAL_ASSERT_EQ(err, WHAL_ETIMEOUT);
    WHAL_ASSERT_EQ(buf[0], 0);
}
#endif /* !WHAL_CFG_NO_TIMEOUT */

void whal_Test_Pl011_Uart(void)
{
    WHAL_TEST_SUITE_START("pl011_uart");
    WHAL_TEST(Test_Pl011_Platform);
    WHAL_TEST(Test_Pl011_Divisors);
    WHAL_TEST(Test_Pl011_DivisorCarry);
    WHAL_TEST(Test_Pl011_LcrhDefault);
    WHAL_TEST(Test_Pl011_Init);
    WHAL_TEST(Test_Pl011_InitLcrhMasked);
    WHAL_TEST(Test_Pl011_InitReprograms);
    WHAL_TEST(Test_Pl011_InitInvalidDivisors);
    WHAL_TEST(Test_Pl011_Deinit);
    WHAL_TEST(Test_Pl011_Send);
    WHAL_TEST(Test_Pl011_Recv);
    WHAL_TEST(Test_Pl011_RecvError);
    WHAL_TEST(Test_Pl011_NullArgs);
    WHAL_TEST(Test_Pl011_Async);
#ifndef WHAL_CFG_NO_TIMEOUT
    WHAL_TEST(Test_Pl011_InitBusyTimeout);
    WHAL_TEST(Test_Pl011_DeinitBusyTimeout);
    WHAL_TEST(Test_Pl011_SendTxffTimeout);
    WHAL_TEST(Test_Pl011_SendDrainTimeout);
    WHAL_TEST(Test_Pl011_RecvTimeout);
#endif
    WHAL_TEST_SUITE_END();
}
