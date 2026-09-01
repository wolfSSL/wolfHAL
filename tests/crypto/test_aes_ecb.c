/* test_aes_ecb.c
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
#include "wolfHAL_board.h"
#include "test.h"

static const uint8_t key[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
};

static const uint8_t plaintext[32] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
};

/* NIST SP 800-38A test vectors (AES-256, single block) */
static const uint8_t nistKey[32] = {
    0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE,
    0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81,
    0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61, 0x08, 0xD7,
    0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4,
};

static const uint8_t nistPt[16] = {
    0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
    0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A,
};

/* NIST SP 800-38A F.1.5 AES-256-ECB expected ciphertext */
static const uint8_t nistEcbCt[16] = {
    0xF3, 0xEE, 0xD1, 0xBD, 0xB5, 0xD2, 0xA0, 0x3C,
    0x06, 0x4B, 0x5A, 0x7E, 0x3D, 0xB1, 0x81, 0xF8,
};

static void Test_AesEcb_Basic(void)
{
    uint8_t ct[32] = {0};
    uint8_t pt[32] = {0};

    WHAL_ASSERT_EQ(whal_AesEcb_Oneshot(BOARD_AES_ECB_DEV, WHAL_CRYPTO_ENCRYPT,
                                        key, 32, plaintext, ct,
                                        sizeof(plaintext)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_AesEcb_Oneshot(BOARD_AES_ECB_DEV, WHAL_CRYPTO_DECRYPT,
                                        key, 32, ct, pt, sizeof(ct)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(pt, plaintext, sizeof(plaintext));
}

static void Test_AesEcb_KnownAnswer(void)
{
    uint8_t ct[16] = {0};

    WHAL_ASSERT_EQ(whal_AesEcb_Oneshot(BOARD_AES_ECB_DEV, WHAL_CRYPTO_ENCRYPT,
                                        nistKey, 32, nistPt, ct,
                                        sizeof(nistPt)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(ct, nistEcbCt, sizeof(nistEcbCt));
}

void whal_Test_AesEcb(void)
{
    WHAL_TEST_SUITE_START("aes_ecb");
    WHAL_TEST(Test_AesEcb_Basic);
    WHAL_TEST(Test_AesEcb_KnownAnswer);
    WHAL_TEST_SUITE_END();
}
