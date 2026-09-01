/* test_spi_loopback.c
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
#include "wolfHAL_board.h"
#include "test.h"

/*
 * Generic SPI loopback test.
 *
 * Requires MOSI wired to MISO so transmitted data is received back.
 * The board must provide g_whalSpi.
 */

static whal_Spi_ComCfg loopbackComCfg = {
    .freq = 1000000,
    .mode = WHAL_SPI_MODE_0,
    .wordSz = 8,
    .dataLines = 1,
};

static void Test_SpiLoopback_SendRecv(void)
{
    uint8_t tx[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t rx[4] = {0};

    WHAL_ASSERT_EQ(whal_Spi_StartCom(BOARD_SPI_DEV, &loopbackComCfg),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV,
                                      tx, sizeof(tx), rx, sizeof(rx)),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Spi_EndCom(BOARD_SPI_DEV), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(rx, tx, sizeof(tx));
}

static void Test_SpiLoopback_NullBufWithLen(void)
{
    uint8_t buf[1] = {0};

    WHAL_ASSERT_EQ(whal_Spi_StartCom(BOARD_SPI_DEV, &loopbackComCfg),
                   WHAL_SUCCESS);

    /* NULL tx with nonzero txLen */
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV, NULL, 1, buf, 1),
                   WHAL_EINVAL);

    /* NULL rx with nonzero rxLen */
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV, buf, 1, NULL, 1),
                   WHAL_EINVAL);

    WHAL_ASSERT_EQ(whal_Spi_EndCom(BOARD_SPI_DEV), WHAL_SUCCESS);
}

static void Test_SpiLoopback_SendRecvDrain(void)
{
    uint8_t tx[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t rx[4] = {0};
    uint8_t expected[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    WHAL_ASSERT_EQ(whal_Spi_StartCom(BOARD_SPI_DEV, &loopbackComCfg),
                   WHAL_SUCCESS);

    /* Send-only: driver must drain RX FIFO internally */
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV, tx, sizeof(tx), NULL, 0),
                   WHAL_SUCCESS);

    /* Receive-only: loopback returns 0xFF (the dummy TX byte) */
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV, NULL, 0, rx, sizeof(rx)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Spi_EndCom(BOARD_SPI_DEV), WHAL_SUCCESS);

    /* If RX wasn't drained, stale 0xDE/0xAD/0xBE/0xEF leaks here */
    WHAL_ASSERT_MEM_EQ(rx, expected, sizeof(expected));
}

static void Test_Spi_Api(void)
{
    WHAL_ASSERT_EQ(whal_Spi_StartCom(BOARD_SPI_DEV, NULL), WHAL_EINVAL);
}

/*
 * Word-size coverage. StartCom doubles as a capability probe: a driver that
 * does not support the width rejects the config and the case is skipped.
 * Each supported width gets the same three checks: an aligned round-trip, a
 * bad-length rejection, and a misaligned-buffer round-trip (frames are held
 * native little-endian in the byte buffer, accessed byte-wise so an odd
 * address must not fault).
 */
static whal_Spi_ComCfg wordSz16ComCfg = {
    .freq = 1000000,
    .mode = WHAL_SPI_MODE_0,
    .wordSz = 16,
    .dataLines = 1,
};

static whal_Spi_ComCfg wordSz32ComCfg = {
    .freq = 1000000,
    .mode = WHAL_SPI_MODE_0,
    .wordSz = 32,
    .dataLines = 1,
};

static void Test_SpiLoopback_WordSz16(void)
{
    uint8_t tx[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t rx[8] = {0};

    if (whal_Spi_StartCom(BOARD_SPI_DEV, &wordSz16ComCfg) != WHAL_SUCCESS)
        WHAL_SKIP();

    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV,
                                      tx, sizeof(tx), rx, sizeof(rx)),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Spi_EndCom(BOARD_SPI_DEV), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(rx, tx, sizeof(tx));
}

static void Test_SpiLoopback_WordSz16OddLen(void)
{
    uint8_t buf[4] = {0};

    if (whal_Spi_StartCom(BOARD_SPI_DEV, &wordSz16ComCfg) != WHAL_SUCCESS)
        WHAL_SKIP();

    /* Length not a whole number of 16-bit frames */
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV, buf, 3, buf, 4),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV, buf, 4, buf, 3),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Spi_EndCom(BOARD_SPI_DEV), WHAL_SUCCESS);
}

static void Test_SpiLoopback_WordSz16Misaligned(void)
{
    uint8_t txRaw[9] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t rxRaw[9] = {0};
    const uint8_t *tx = txRaw + 1;   /* odd address */
    uint8_t *rx = rxRaw + 1;

    if (whal_Spi_StartCom(BOARD_SPI_DEV, &wordSz16ComCfg) != WHAL_SUCCESS)
        WHAL_SKIP();

    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV, tx, 8, rx, 8),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Spi_EndCom(BOARD_SPI_DEV), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(rx, tx, 8);
}

static void Test_SpiLoopback_WordSz32(void)
{
    uint8_t tx[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t rx[8] = {0};

    if (whal_Spi_StartCom(BOARD_SPI_DEV, &wordSz32ComCfg) != WHAL_SUCCESS)
        WHAL_SKIP();

    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV,
                                      tx, sizeof(tx), rx, sizeof(rx)),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Spi_EndCom(BOARD_SPI_DEV), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(rx, tx, sizeof(tx));
}

static void Test_SpiLoopback_WordSz32OddLen(void)
{
    uint8_t buf[8] = {0};

    if (whal_Spi_StartCom(BOARD_SPI_DEV, &wordSz32ComCfg) != WHAL_SUCCESS)
        WHAL_SKIP();

    /* Length not a whole number of 32-bit frames */
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV, buf, 3, buf, 8),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV, buf, 8, buf, 3),
                   WHAL_EINVAL);
    WHAL_ASSERT_EQ(whal_Spi_EndCom(BOARD_SPI_DEV), WHAL_SUCCESS);
}

static void Test_SpiLoopback_WordSz32Misaligned(void)
{
    uint8_t txRaw[9] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t rxRaw[9] = {0};
    const uint8_t *tx = txRaw + 1;   /* odd address */
    uint8_t *rx = rxRaw + 1;

    if (whal_Spi_StartCom(BOARD_SPI_DEV, &wordSz32ComCfg) != WHAL_SUCCESS)
        WHAL_SKIP();

    WHAL_ASSERT_EQ(whal_Spi_SendRecv(BOARD_SPI_DEV, tx, 8, rx, 8),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Spi_EndCom(BOARD_SPI_DEV), WHAL_SUCCESS);
    WHAL_ASSERT_MEM_EQ(rx, tx, 8);
}

void whal_Test_Spi_Loopback(void)
{
    WHAL_TEST_SUITE_START("spi_loopback");
    WHAL_TEST(Test_Spi_Api);
    WHAL_TEST(Test_SpiLoopback_SendRecv);
    WHAL_TEST(Test_SpiLoopback_NullBufWithLen);
    WHAL_TEST(Test_SpiLoopback_SendRecvDrain);
    WHAL_TEST(Test_SpiLoopback_WordSz16);
    WHAL_TEST(Test_SpiLoopback_WordSz16OddLen);
    WHAL_TEST(Test_SpiLoopback_WordSz16Misaligned);
    WHAL_TEST(Test_SpiLoopback_WordSz32);
    WHAL_TEST(Test_SpiLoopback_WordSz32OddLen);
    WHAL_TEST(Test_SpiLoopback_WordSz32Misaligned);
    WHAL_TEST_SUITE_END();
}
