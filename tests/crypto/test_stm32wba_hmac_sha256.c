/* test_stm32wba_hmac_sha256.c
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
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/crypto/stm32wba_hash.h>
#include "wolfHAL_board.h"
#include "test.h"

#define HASH_CR_REG          0x00
#define HASH_CR_INIT_Msk     (1UL << 2)
#define HASH_SR_REG          0x24
#define HASH_SR_DINIS_Msk    (1UL << 0)
#define HASH_HR_REG(x)       (0x310 + (x) * 4)
#define HASH_CSR_REG(x)      (0xF8 + (x) * 4)

#define HASH_CSR_COUNT       54

static const uint8_t hmacKey[20] = {
    0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
    0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
    0x0B, 0x0B, 0x0B, 0x0B,
};

static void ReadCsrs(uintptr_t base, uint32_t *out, size_t first, size_t last)
{
    size_t i;

    for (i = first; i < last; i++)
        out[i - first] = (uint32_t)whal_Reg32_Read(base, HASH_CSR_REG(i));
}

static int AnyNonZero(const uint32_t *words, size_t count)
{
    size_t i;

    for (i = 0; i < count; i++) {
        if (words[i] != 0)
            return 1;
    }
    return 0;
}

static void Test_Hash_InitChangesHmacContext(void)
{
    uintptr_t base = whal_Stm32wba_Hash_Dev.base;
    uint32_t before[HASH_CSR_COUNT];
    uint32_t after[HASH_CSR_COUNT];

    WHAL_ASSERT_EQ(whal_HmacSha256_Start(BOARD_HMAC_SHA256_DEV,
                                         hmacKey, sizeof(hmacKey)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Reg32_ReadPoll(base, HASH_SR_REG, HASH_SR_DINIS_Msk,
                                       HASH_SR_DINIS_Msk, &g_whalTimeout),
                   WHAL_SUCCESS);
    ReadCsrs(base, before, 0, HASH_CSR_COUNT);
    WHAL_ASSERT_EQ(AnyNonZero(before, HASH_CSR_COUNT), 1);

    whal_Reg32_Update(base, HASH_CR_REG, HASH_CR_INIT_Msk, HASH_CR_INIT_Msk);

    if ((whal_Reg32_Read(base, HASH_SR_REG) & HASH_SR_DINIS_Msk) == 0)
        WHAL_SKIP();

    ReadCsrs(base, after, 0, HASH_CSR_COUNT);
    WHAL_ASSERT_MEM_NEQ(after, before, sizeof(before));
}

static void Test_Hash_DeinitClearsContext(void)
{
    uintptr_t base = whal_Stm32wba_Hash_Dev.base;
    uint32_t csr[HASH_CSR_COUNT];
    size_t i;

    WHAL_ASSERT_EQ(whal_HmacSha256_Start(BOARD_HMAC_SHA256_DEV,
                                         hmacKey, sizeof(hmacKey)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Stm32wba_Hash_Deinit(
                       (whal_Crypto *)&whal_Stm32wba_Hash_Dev),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_Reg32_Read(base, HASH_SR_REG) & HASH_SR_DINIS_Msk, 0);
    ReadCsrs(base, csr, 0, HASH_CSR_COUNT);
    WHAL_ASSERT_EQ(AnyNonZero(csr, HASH_CSR_COUNT), 0);

    for (i = 0; i < 8; i++)
        WHAL_ASSERT_EQ(whal_Reg32_Read(base, HASH_HR_REG(i)), 0);

    WHAL_ASSERT_EQ(whal_Stm32wba_Hash_Init(
                       (whal_Crypto *)&whal_Stm32wba_Hash_Dev),
                   WHAL_SUCCESS);
}

void whal_Test_HmacSha256_Platform(void)
{
    WHAL_TEST_SUITE_START("hmac_sha256_platform");
    WHAL_TEST(Test_Hash_InitChangesHmacContext);
    WHAL_TEST(Test_Hash_DeinitClearsContext);
    WHAL_TEST_SUITE_END();
}
