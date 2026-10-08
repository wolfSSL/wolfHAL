/* test_endian.c
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

#include <wolfHAL/endian.h>
#include "../test.h"

static void Test_Endian_LoadBe32(void)
{
    const uint8_t buf[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    WHAL_ASSERT_EQ(whal_LoadBe32(buf), 0xDEADBEEFul);
}

static void Test_Endian_LoadBe32_Zero(void)
{
    const uint8_t buf[] = { 0x00, 0x00, 0x00, 0x00 };
    WHAL_ASSERT_EQ(whal_LoadBe32(buf), 0x00000000ul);
}

static void Test_Endian_LoadBe32_AllOnes(void)
{
    const uint8_t buf[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    WHAL_ASSERT_EQ(whal_LoadBe32(buf), 0xFFFFFFFFul);
}

static void Test_Endian_LoadBe32_MsbOnly(void)
{
    const uint8_t buf[] = { 0x80, 0x00, 0x00, 0x00 };
    WHAL_ASSERT_EQ(whal_LoadBe32(buf), 0x80000000ul);
}

static void Test_Endian_LoadBe32_LsbOnly(void)
{
    const uint8_t buf[] = { 0x00, 0x00, 0x00, 0x01 };
    WHAL_ASSERT_EQ(whal_LoadBe32(buf), 0x00000001ul);
}

static void Test_Endian_LoadLe32(void)
{
    const uint8_t buf[] = { 0xEF, 0xBE, 0xAD, 0xDE };
    WHAL_ASSERT_EQ(whal_LoadLe32(buf), 0xDEADBEEFul);
}

static void Test_Endian_LoadLe32_Zero(void)
{
    const uint8_t buf[] = { 0x00, 0x00, 0x00, 0x00 };
    WHAL_ASSERT_EQ(whal_LoadLe32(buf), 0x00000000ul);
}

static void Test_Endian_LoadLe32_AllOnes(void)
{
    const uint8_t buf[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    WHAL_ASSERT_EQ(whal_LoadLe32(buf), 0xFFFFFFFFul);
}

static void Test_Endian_LoadLe32_MsbOnly(void)
{
    const uint8_t buf[] = { 0x00, 0x00, 0x00, 0x80 };
    WHAL_ASSERT_EQ(whal_LoadLe32(buf), 0x80000000ul);
}

static void Test_Endian_LoadLe32_LsbOnly(void)
{
    const uint8_t buf[] = { 0x01, 0x00, 0x00, 0x00 };
    WHAL_ASSERT_EQ(whal_LoadLe32(buf), 0x00000001ul);
}

static void Test_Endian_StoreBe32(void)
{
    uint8_t buf[4] = {0};
    whal_StoreBe32(buf, 0xDEADBEEF);
    WHAL_ASSERT_EQ(buf[0], 0xDE);
    WHAL_ASSERT_EQ(buf[1], 0xAD);
    WHAL_ASSERT_EQ(buf[2], 0xBE);
    WHAL_ASSERT_EQ(buf[3], 0xEF);
}

static void Test_Endian_StoreBe32_Zero(void)
{
    uint8_t buf[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    whal_StoreBe32(buf, 0x00000000);
    WHAL_ASSERT_EQ(buf[0], 0x00);
    WHAL_ASSERT_EQ(buf[1], 0x00);
    WHAL_ASSERT_EQ(buf[2], 0x00);
    WHAL_ASSERT_EQ(buf[3], 0x00);
}

static void Test_Endian_StoreBe32_MsbOnly(void)
{
    uint8_t buf[4] = {0};
    whal_StoreBe32(buf, 0x80000000);
    WHAL_ASSERT_EQ(buf[0], 0x80);
    WHAL_ASSERT_EQ(buf[1], 0x00);
    WHAL_ASSERT_EQ(buf[2], 0x00);
    WHAL_ASSERT_EQ(buf[3], 0x00);
}

static void Test_Endian_Roundtrip(void)
{
    uint8_t buf[4] = {0};
    whal_StoreBe32(buf, 0xCAFEBABE);
    WHAL_ASSERT_EQ(whal_LoadBe32(buf), 0xCAFEBABEul);
}

void whal_Test_Endian(void)
{
    WHAL_TEST_SUITE_START("endian");
    WHAL_TEST(Test_Endian_LoadBe32);
    WHAL_TEST(Test_Endian_LoadBe32_Zero);
    WHAL_TEST(Test_Endian_LoadBe32_AllOnes);
    WHAL_TEST(Test_Endian_LoadBe32_MsbOnly);
    WHAL_TEST(Test_Endian_LoadBe32_LsbOnly);
    WHAL_TEST(Test_Endian_LoadLe32);
    WHAL_TEST(Test_Endian_LoadLe32_Zero);
    WHAL_TEST(Test_Endian_LoadLe32_AllOnes);
    WHAL_TEST(Test_Endian_LoadLe32_MsbOnly);
    WHAL_TEST(Test_Endian_LoadLe32_LsbOnly);
    WHAL_TEST(Test_Endian_StoreBe32);
    WHAL_TEST(Test_Endian_StoreBe32_Zero);
    WHAL_TEST(Test_Endian_StoreBe32_MsbOnly);
    WHAL_TEST(Test_Endian_Roundtrip);
    WHAL_TEST_SUITE_END();
}
