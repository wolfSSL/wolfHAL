/* test_aes_gcm.c
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

static const uint8_t nonce[12] = {
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
    0xB8, 0xB9, 0xBA, 0xBB,
};

static const uint8_t aad[16] = {
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
};

static const uint8_t plaintext[32] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
};

/* GCM spec Test Case 15: AES-256-GCM, 64-byte payload, no AAD */
static const uint8_t gcmKey[32] = {
    0xFE, 0xFF, 0xE9, 0x92, 0x86, 0x65, 0x73, 0x1C,
    0x6D, 0x6A, 0x8F, 0x94, 0x67, 0x30, 0x83, 0x08,
    0xFE, 0xFF, 0xE9, 0x92, 0x86, 0x65, 0x73, 0x1C,
    0x6D, 0x6A, 0x8F, 0x94, 0x67, 0x30, 0x83, 0x08,
};

static const uint8_t gcmIv[12] = {
    0xCA, 0xFE, 0xBA, 0xBE, 0xFA, 0xCE, 0xDB, 0xAD,
    0xDE, 0xCA, 0xF8, 0x88,
};

static const uint8_t gcmPt[64] = {
    0xD9, 0x31, 0x32, 0x25, 0xF8, 0x84, 0x06, 0xE5,
    0xA5, 0x59, 0x09, 0xC5, 0xAF, 0xF5, 0x26, 0x9A,
    0x86, 0xA7, 0xA9, 0x53, 0x15, 0x34, 0xF7, 0xDA,
    0x2E, 0x4C, 0x30, 0x3D, 0x8A, 0x31, 0x8A, 0x72,
    0x1C, 0x3C, 0x0C, 0x95, 0x95, 0x68, 0x09, 0x53,
    0x2F, 0xCF, 0x0E, 0x24, 0x49, 0xA6, 0xB5, 0x25,
    0xB1, 0x6A, 0xED, 0xF5, 0xAA, 0x0D, 0xE6, 0x57,
    0xBA, 0x63, 0x7B, 0x39, 0x1A, 0xAF, 0xD2, 0x55,
};

static const uint8_t gcmCt[64] = {
    0x52, 0x2D, 0xC1, 0xF0, 0x99, 0x56, 0x7D, 0x07,
    0xF4, 0x7F, 0x37, 0xA3, 0x2A, 0x84, 0x42, 0x7D,
    0x64, 0x3A, 0x8C, 0xDC, 0xBF, 0xE5, 0xC0, 0xC9,
    0x75, 0x98, 0xA2, 0xBD, 0x25, 0x55, 0xD1, 0xAA,
    0x8C, 0xB0, 0x8E, 0x48, 0x59, 0x0D, 0xBB, 0x3D,
    0xA7, 0xB0, 0x8B, 0x10, 0x56, 0x82, 0x88, 0x38,
    0xC5, 0xF6, 0x1E, 0x63, 0x93, 0xBA, 0x7A, 0x0A,
    0xBC, 0xC9, 0xF6, 0x62, 0x89, 0x80, 0x15, 0xAD,
};

static const uint8_t gcmTag[16] = {
    0xB0, 0x94, 0xDA, 0xC5, 0xD9, 0x34, 0x71, 0xBD,
    0xEC, 0x1A, 0x50, 0x22, 0x70, 0xE3, 0xCC, 0x6C,
};

/* GCM spec Test Case 16: AES-256-GCM, 60-byte payload, 20-byte AAD.
 * Exercises partial last blocks on both AAD and payload. Reuses gcmKey/gcmIv. */
static const uint8_t gcm16Pt[60] = {
    0xD9, 0x31, 0x32, 0x25, 0xF8, 0x84, 0x06, 0xE5,
    0xA5, 0x59, 0x09, 0xC5, 0xAF, 0xF5, 0x26, 0x9A,
    0x86, 0xA7, 0xA9, 0x53, 0x15, 0x34, 0xF7, 0xDA,
    0x2E, 0x4C, 0x30, 0x3D, 0x8A, 0x31, 0x8A, 0x72,
    0x1C, 0x3C, 0x0C, 0x95, 0x95, 0x68, 0x09, 0x53,
    0x2F, 0xCF, 0x0E, 0x24, 0x49, 0xA6, 0xB5, 0x25,
    0xB1, 0x6A, 0xED, 0xF5, 0xAA, 0x0D, 0xE6, 0x57,
    0xBA, 0x63, 0x7B, 0x39,
};

static const uint8_t gcm16Aad[20] = {
    0xFE, 0xED, 0xFA, 0xCE, 0xDE, 0xAD, 0xBE, 0xEF,
    0xFE, 0xED, 0xFA, 0xCE, 0xDE, 0xAD, 0xBE, 0xEF,
    0xAB, 0xAD, 0xDA, 0xD2,
};

static const uint8_t gcm16Ct[60] = {
    0x52, 0x2D, 0xC1, 0xF0, 0x99, 0x56, 0x7D, 0x07,
    0xF4, 0x7F, 0x37, 0xA3, 0x2A, 0x84, 0x42, 0x7D,
    0x64, 0x3A, 0x8C, 0xDC, 0xBF, 0xE5, 0xC0, 0xC9,
    0x75, 0x98, 0xA2, 0xBD, 0x25, 0x55, 0xD1, 0xAA,
    0x8C, 0xB0, 0x8E, 0x48, 0x59, 0x0D, 0xBB, 0x3D,
    0xA7, 0xB0, 0x8B, 0x10, 0x56, 0x82, 0x88, 0x38,
    0xC5, 0xF6, 0x1E, 0x63, 0x93, 0xBA, 0x7A, 0x0A,
    0xBC, 0xC9, 0xF6, 0x62,
};

static const uint8_t gcm16Tag[16] = {
    0x76, 0xFC, 0x6E, 0xCE, 0x0F, 0x4E, 0x17, 0x68,
    0xCD, 0xDF, 0x88, 0x53, 0xBB, 0x2D, 0x55, 0x1B,
};

static void Test_AesGcm_Basic(void)
{
    uint8_t ct[32] = {0};
    uint8_t pt[32] = {0};
    uint8_t encTag[16] = {0};
    uint8_t decTag[16] = {0};

    WHAL_ASSERT_EQ(whal_AesGcm_Oneshot(BOARD_AES_GCM_DEV, WHAL_CRYPTO_ENCRYPT,
                                        key, 32,
                                        nonce, sizeof(nonce),
                                        aad, sizeof(aad),
                                        plaintext, ct, sizeof(plaintext),
                                        encTag, sizeof(encTag)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_AesGcm_Oneshot(BOARD_AES_GCM_DEV, WHAL_CRYPTO_DECRYPT,
                                        key, 32,
                                        nonce, sizeof(nonce),
                                        aad, sizeof(aad),
                                        ct, pt, sizeof(ct),
                                        decTag, sizeof(decTag)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(pt, plaintext, sizeof(plaintext));
    WHAL_ASSERT_MEM_EQ(decTag, encTag, sizeof(encTag));
}

static void Test_AesGcm_KnownAnswer(void)
{
    uint8_t ct[64] = {0};
    uint8_t tag[16] = {0};

    WHAL_ASSERT_EQ(whal_AesGcm_Oneshot(BOARD_AES_GCM_DEV, WHAL_CRYPTO_ENCRYPT,
                                        gcmKey, 32,
                                        gcmIv, sizeof(gcmIv),
                                        NULL, 0,
                                        gcmPt, ct, sizeof(gcmPt),
                                        tag, sizeof(tag)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(ct, gcmCt, sizeof(gcmCt));
    WHAL_ASSERT_MEM_EQ(tag, gcmTag, sizeof(gcmTag));
}

static void Test_AesGcm_KnownAnswer_PartialBlocks(void)
{
    uint8_t ct[sizeof(gcm16Pt)] = {0};
    uint8_t tag[16] = {0};

    WHAL_ASSERT_EQ(whal_AesGcm_Oneshot(BOARD_AES_GCM_DEV, WHAL_CRYPTO_ENCRYPT,
                                        gcmKey, 32,
                                        gcmIv, sizeof(gcmIv),
                                        gcm16Aad, sizeof(gcm16Aad),
                                        gcm16Pt, ct, sizeof(gcm16Pt),
                                        tag, sizeof(tag)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(ct, gcm16Ct, sizeof(gcm16Ct));
    WHAL_ASSERT_MEM_EQ(tag, gcm16Tag, sizeof(gcm16Tag));
}

/* Streaming KAT vectors generated by tests/crypto/gen_test_vectors.py from
 * gcmKey + gcmIv above, AEAD_PT (50-byte deterministic plaintext) and
 * AEAD_AAD (20-byte deterministic AAD). Re-run the script if these inputs
 * are changed. */
static const uint8_t gcmStreamAad[20] = {
    0x05, 0x10, 0x1B, 0x26, 0x31, 0x3C, 0x47, 0x52,
    0x5D, 0x68, 0x73, 0x7E, 0x89, 0x94, 0x9F, 0xAA,
    0xB5, 0xC0, 0xCB, 0xD6,
};

static const uint8_t gcmStreamPt[50] = {
    0x03, 0x0A, 0x11, 0x18, 0x1F, 0x26, 0x2D, 0x34,
    0x3B, 0x42, 0x49, 0x50, 0x57, 0x5E, 0x65, 0x6C,
    0x73, 0x7A, 0x81, 0x88, 0x8F, 0x96, 0x9D, 0xA4,
    0xAB, 0xB2, 0xB9, 0xC0, 0xC7, 0xCE, 0xD5, 0xDC,
    0xE3, 0xEA, 0xF1, 0xF8, 0xFF, 0x06, 0x0D, 0x14,
    0x1B, 0x22, 0x29, 0x30, 0x37, 0x3E, 0x45, 0x4C,
    0x53, 0x5A,
};

static const uint8_t gcmStreamCt[50] = {
    0x88, 0x16, 0xE2, 0xCD, 0x7E, 0xF4, 0x56, 0xD6,
    0x6A, 0x64, 0x77, 0x36, 0xD2, 0x2F, 0x01, 0x8B,
    0x91, 0xE7, 0xA4, 0x07, 0x25, 0x47, 0xAA, 0xB7,
    0xF0, 0x66, 0x2B, 0x40, 0x68, 0xAA, 0x8E, 0x04,
    0x73, 0x66, 0x73, 0x25, 0x33, 0x63, 0xBF, 0x7A,
    0x93, 0x5D, 0xAC, 0x04, 0x28, 0x1A, 0x78, 0x51,
    0x27, 0xC6,
};

static const uint8_t gcmStreamTag[16] = {
    0xDF, 0xB6, 0x81, 0x09, 0xD6, 0xD1, 0x0A, 0xEE,
    0x2E, 0xA9, 0xBD, 0x9F, 0x9C, 0x4E, 0x63, 0x97,
};

static const uint8_t gcmStreamNoAadTag[16] = {
    0x27, 0x99, 0x9F, 0xE0, 0xEB, 0xA9, 0x16, 0x72,
    0x50, 0x6F, 0xD9, 0xD0, 0x8F, 0x4E, 0x3F, 0x9A,
};

static const uint8_t gcmStreamNoPtTag[16] = {
    0x84, 0x16, 0xDF, 0xFE, 0x20, 0x34, 0xE6, 0xE6,
    0xD3, 0xB6, 0x6B, 0xE7, 0x99, 0x55, 0xB4, 0x56,
};

static void Test_AesGcm_Streaming(void)
{
    uint8_t ct[sizeof(gcmStreamPt)] = {0};
    uint8_t tag[16] = {0};
    const size_t split = 32;

    WHAL_ASSERT_EQ(whal_AesGcm_Start(BOARD_AES_GCM_DEV, WHAL_CRYPTO_ENCRYPT,
                                      gcmKey, 32,
                                      gcmIv, sizeof(gcmIv),
                                      gcmStreamAad, sizeof(gcmStreamAad)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_AesGcm_Process(BOARD_AES_GCM_DEV,
                                        gcmStreamPt, ct, split),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_AesGcm_Process(BOARD_AES_GCM_DEV,
                                        gcmStreamPt + split, ct + split,
                                        sizeof(gcmStreamPt) - split),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_AesGcm_Finalize(BOARD_AES_GCM_DEV, tag, sizeof(tag)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(ct, gcmStreamCt, sizeof(gcmStreamCt));
    WHAL_ASSERT_MEM_EQ(tag, gcmStreamTag, sizeof(gcmStreamTag));
}

static void Test_AesGcm_Streaming_NoAad(void)
{
    uint8_t ct[sizeof(gcmStreamPt)] = {0};
    uint8_t tag[16] = {0};
    const size_t split = 32;

    WHAL_ASSERT_EQ(whal_AesGcm_Start(BOARD_AES_GCM_DEV, WHAL_CRYPTO_ENCRYPT,
                                      gcmKey, 32,
                                      gcmIv, sizeof(gcmIv),
                                      NULL, 0),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_AesGcm_Process(BOARD_AES_GCM_DEV,
                                        gcmStreamPt, ct, split),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_AesGcm_Process(BOARD_AES_GCM_DEV,
                                        gcmStreamPt + split, ct + split,
                                        sizeof(gcmStreamPt) - split),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_AesGcm_Finalize(BOARD_AES_GCM_DEV, tag, sizeof(tag)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(ct, gcmStreamCt, sizeof(gcmStreamCt));
    WHAL_ASSERT_MEM_EQ(tag, gcmStreamNoAadTag, sizeof(gcmStreamNoAadTag));
}

static void Test_AesGcm_Streaming_NoPayload(void)
{
    uint8_t tag[16] = {0};

    WHAL_ASSERT_EQ(whal_AesGcm_Start(BOARD_AES_GCM_DEV, WHAL_CRYPTO_ENCRYPT,
                                      gcmKey, 32,
                                      gcmIv, sizeof(gcmIv),
                                      gcmStreamAad, sizeof(gcmStreamAad)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_AesGcm_Finalize(BOARD_AES_GCM_DEV, tag, sizeof(tag)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(tag, gcmStreamNoPtTag, sizeof(gcmStreamNoPtTag));
}

static void Test_AesGcm_Streaming_ThreeCall(void)
{
    uint8_t ct[sizeof(gcmStreamPt)] = {0};
    uint8_t tag[16] = {0};

    WHAL_ASSERT_EQ(whal_AesGcm_Start(BOARD_AES_GCM_DEV, WHAL_CRYPTO_ENCRYPT,
                                      gcmKey, 32,
                                      gcmIv, sizeof(gcmIv),
                                      gcmStreamAad, sizeof(gcmStreamAad)),
                   WHAL_SUCCESS);

    /* 16 + 16 + 18: two full blocks then one block + 2 partial. */
    WHAL_ASSERT_EQ(whal_AesGcm_Process(BOARD_AES_GCM_DEV,
                                        gcmStreamPt, ct, 16),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesGcm_Process(BOARD_AES_GCM_DEV,
                                        gcmStreamPt + 16, ct + 16, 16),
                   WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_AesGcm_Process(BOARD_AES_GCM_DEV,
                                        gcmStreamPt + 32, ct + 32,
                                        sizeof(gcmStreamPt) - 32),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(whal_AesGcm_Finalize(BOARD_AES_GCM_DEV, tag, sizeof(tag)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(ct, gcmStreamCt, sizeof(gcmStreamCt));
    WHAL_ASSERT_MEM_EQ(tag, gcmStreamTag, sizeof(gcmStreamTag));
}

void whal_Test_AesGcm(void)
{
    WHAL_TEST_SUITE_START("aes_gcm");
    WHAL_TEST(Test_AesGcm_Basic);
    WHAL_TEST(Test_AesGcm_KnownAnswer);
    WHAL_TEST(Test_AesGcm_KnownAnswer_PartialBlocks);
    WHAL_TEST(Test_AesGcm_Streaming);
    WHAL_TEST(Test_AesGcm_Streaming_NoAad);
    WHAL_TEST(Test_AesGcm_Streaming_NoPayload);
    WHAL_TEST(Test_AesGcm_Streaming_ThreeCall);
    WHAL_TEST_SUITE_END();
}
