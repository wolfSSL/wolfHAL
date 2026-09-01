/* stm32wb_aes.c
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
#include "wolfHAL_board.h"  /* provides WHAL_CFG_STM32WB_AES*_DEV initializers */
#include <wolfHAL/crypto/stm32wb_aes.h>
#include <wolfHAL/crypto/crypto.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/endian.h>

/* Mutable state buffers for GCM/CCM streaming sessions. Internal linkage —
 * referenced only by the AesGcm/AesCcm singletons below. */
static whal_Stm32wb_AesGcm_State g_stm32wbAesGcmDevState;
static whal_Stm32wb_AesCcm_State g_stm32wbAesCcmDevState;

const whal_Crypto  whal_Stm32wb_Aes_Dev      = WHAL_CFG_STM32WB_AES_DEV;
const whal_AesEcb  whal_Stm32wb_AesEcb_Dev   = WHAL_CFG_STM32WB_AES_ECB_DEV;
const whal_AesCbc  whal_Stm32wb_AesCbc_Dev   = WHAL_CFG_STM32WB_AES_CBC_DEV;
const whal_AesCtr  whal_Stm32wb_AesCtr_Dev   = WHAL_CFG_STM32WB_AES_CTR_DEV;
const whal_AesGcm  whal_Stm32wb_AesGcm_Dev   = WHAL_CFG_STM32WB_AES_GCM_DEV;
const whal_AesGmac whal_Stm32wb_AesGmac_Dev  = WHAL_CFG_STM32WB_AES_GMAC_DEV;
const whal_AesCcm  whal_Stm32wb_AesCcm_Dev   = WHAL_CFG_STM32WB_AES_CCM_DEV;

/* Control Register */
#define AES_CR_REG        0x00
#define AES_CR_EN_Pos     0
#define AES_CR_EN_Msk     (1UL << AES_CR_EN_Pos)

#define AES_CR_DATATYPE_Pos 1
#define AES_CR_DATATYPE_Msk (3UL << AES_CR_DATATYPE_Pos)

#define AES_CR_MODE_Pos   3
#define AES_CR_MODE_Msk   (3UL << AES_CR_MODE_Pos)

#define AES_CR_CHMOD_Pos  5
#define AES_CR_CHMOD_Msk  (3UL << AES_CR_CHMOD_Pos)

#define AES_CR_CHMOD2_Pos 16
#define AES_CR_CHMOD2_Msk (1UL << AES_CR_CHMOD2_Pos)

#define AES_CR_CCFC_Pos   7
#define AES_CR_CCFC_Msk   (1UL << AES_CR_CCFC_Pos)

#define AES_CR_KEYSIZE_Pos 18
#define AES_CR_KEYSIZE_Msk (1UL << AES_CR_KEYSIZE_Pos)

#define AES_CR_GCMPH_Pos  13
#define AES_CR_GCMPH_Msk  (3UL << AES_CR_GCMPH_Pos)

#define AES_CR_NPBLB_Pos  20
#define AES_CR_NPBLB_Msk  (0xF << AES_CR_NPBLB_Pos)

/* Status Register */
#define AES_SR_REG        0x04
#define AES_SR_CCF_Pos    0
#define AES_SR_CCF_Msk    (1UL << AES_SR_CCF_Pos)

#define AES_SR_RDERR_Pos  1
#define AES_SR_RDERR_Msk  (1UL << AES_SR_RDERR_Pos)

#define AES_SR_WRERR_Pos  2
#define AES_SR_WRERR_Msk  (1UL << AES_SR_WRERR_Pos)

/* Data Registers */
#define AES_DINR_REG      0x08
#define AES_DOUTR_REG     0x0C

/* Key Registers */
#define AES_KEYR0_REG     0x10
#define AES_KEYR1_REG     0x14
#define AES_KEYR2_REG     0x18
#define AES_KEYR3_REG     0x1C
#define AES_KEYR4_REG     0x30
#define AES_KEYR5_REG     0x34
#define AES_KEYR6_REG     0x38
#define AES_KEYR7_REG     0x3C

/* Initialization Vector Registers */
#define AES_IVR0_REG      0x20
#define AES_IVR1_REG      0x24
#define AES_IVR2_REG      0x28
#define AES_IVR3_REG      0x2C

/* Chaining modes */
#define AES_CHMOD_ECB     0x0
#define AES_CHMOD_CBC     0x1
#define AES_CHMOD_CTR     0x2
#define AES_CHMOD_GCM     0x3
#define AES_CHMOD_CCM     0x4

/* Operating modes */
#define AES_MODE_ENCRYPT  0x0
#define AES_MODE_KEYDERIV 0x1
#define AES_MODE_DECRYPT  0x2
#define AES_MODE_KEYDERIV_DECRYPT 0x3

/* Data types (swap modes) */
#define AES_DATATYPE_NONE     0x0
#define AES_DATATYPE_HALFWORD 0x1
#define AES_DATATYPE_BYTE     0x2
#define AES_DATATYPE_BIT      0x3

/* GCM phases */
#define AES_GCMPH_INIT    0x0
#define AES_GCMPH_HEADER  0x1
#define AES_GCMPH_PAYLOAD 0x2
#define AES_GCMPH_FINAL   0x3

static void WriteKey(size_t base, const uint8_t *key, size_t keySz)
{
    const uint8_t *k = key;
    if (keySz == 32) {
        whal_Reg_Write(base, AES_KEYR7_REG, whal_LoadBe32(k));
        whal_Reg_Write(base, AES_KEYR6_REG, whal_LoadBe32(k + 4));
        whal_Reg_Write(base, AES_KEYR5_REG, whal_LoadBe32(k + 8));
        whal_Reg_Write(base, AES_KEYR4_REG, whal_LoadBe32(k + 12));
        k += 16;
    }
    whal_Reg_Write(base, AES_KEYR3_REG, whal_LoadBe32(k));
    whal_Reg_Write(base, AES_KEYR2_REG, whal_LoadBe32(k + 4));
    whal_Reg_Write(base, AES_KEYR1_REG, whal_LoadBe32(k + 8));
    whal_Reg_Write(base, AES_KEYR0_REG, whal_LoadBe32(k + 12));
}

static void WriteIv(size_t base, const uint8_t *iv)
{
    whal_Reg_Write(base, AES_IVR3_REG, whal_LoadBe32(iv));
    whal_Reg_Write(base, AES_IVR2_REG, whal_LoadBe32(iv + 4));
    whal_Reg_Write(base, AES_IVR1_REG, whal_LoadBe32(iv + 8));
    whal_Reg_Write(base, AES_IVR0_REG, whal_LoadBe32(iv + 12));
}

/* Wipe key and IV/counter material from the peripheral registers. Callers must
 * clear EN first (key registers are only writable while disabled). */
static void ZeroKeyIv(size_t base)
{
    whal_Reg_Write(base, AES_KEYR7_REG, 0);
    whal_Reg_Write(base, AES_KEYR6_REG, 0);
    whal_Reg_Write(base, AES_KEYR5_REG, 0);
    whal_Reg_Write(base, AES_KEYR4_REG, 0);
    whal_Reg_Write(base, AES_KEYR3_REG, 0);
    whal_Reg_Write(base, AES_KEYR2_REG, 0);
    whal_Reg_Write(base, AES_KEYR1_REG, 0);
    whal_Reg_Write(base, AES_KEYR0_REG, 0);
    whal_Reg_Write(base, AES_IVR3_REG, 0);
    whal_Reg_Write(base, AES_IVR2_REG, 0);
    whal_Reg_Write(base, AES_IVR1_REG, 0);
    whal_Reg_Write(base, AES_IVR0_REG, 0);
}

static void WriteBlock(size_t base, const uint8_t *in)
{
    whal_Reg_Write(base, AES_DINR_REG, whal_LoadBe32(in));
    whal_Reg_Write(base, AES_DINR_REG, whal_LoadBe32(in + 4));
    whal_Reg_Write(base, AES_DINR_REG, whal_LoadBe32(in + 8));
    whal_Reg_Write(base, AES_DINR_REG, whal_LoadBe32(in + 12));
}

static void ReadBlock(size_t base, uint8_t *out)
{
    whal_StoreBe32(out,      whal_Reg_Read(base, AES_DOUTR_REG));
    whal_StoreBe32(out + 4,  whal_Reg_Read(base, AES_DOUTR_REG));
    whal_StoreBe32(out + 8,  whal_Reg_Read(base, AES_DOUTR_REG));
    whal_StoreBe32(out + 12, whal_Reg_Read(base, AES_DOUTR_REG));
}

static whal_Error WaitForCCF(size_t base, whal_Timeout *timeout)
{
    whal_Error err;
    err = whal_Reg_ReadPoll(base, AES_SR_REG, AES_SR_CCF_Msk,
                            AES_SR_CCF_Msk, timeout);
    if (err)
        return err;
    whal_Reg_Update(base, AES_CR_REG, AES_CR_CCFC_Msk, AES_CR_CCFC_Msk);
    return WHAL_SUCCESS;
}

static whal_Error ProcessBlockCipher(whal_Crypto *cryptoDev,
                                     const uint8_t *in, uint8_t *out,
                                     size_t sz)
{
    const whal_Stm32wb_Aes_Cfg *cfg =
        (const whal_Stm32wb_Aes_Cfg *)cryptoDev->cfg;
    size_t base = cryptoDev->base;
    whal_Error err;
    size_t i;

    if (sz == 0)
        return WHAL_SUCCESS;

    if (!in || !out || (sz & 0xF) != 0) {
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
        ZeroKeyIv(base);
        return WHAL_EINVAL;
    }

    for (i = 0; i < sz; i += 16) {
        WriteBlock(base, in + i);
        err = WaitForCCF(base, cfg->timeout);
        if (err) {
            whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
            ZeroKeyIv(base);
            return err;
        }
        ReadBlock(base, out + i);
    }

    return WHAL_SUCCESS;
}

static void DisableAes(whal_Crypto *cryptoDev)
{
    whal_Reg_Update(cryptoDev->base, AES_CR_REG, AES_CR_EN_Msk, 0);
}


/* ---- Direct API mapping ---- */

#ifdef WHAL_CFG_STM32WB_AES_INIT_DIRECT_API_MAPPING
#define whal_Stm32wb_Aes_Init       whal_Crypto_Init
#define whal_Stm32wb_Aes_Deinit     whal_Crypto_Deinit
#endif
#ifdef WHAL_CFG_STM32WB_AES_ECB_DIRECT_API_MAPPING
#define whal_Stm32wb_AesEcb_Oneshot whal_AesEcb_Oneshot
#define whal_Stm32wb_AesEcb_Start   whal_AesEcb_Start
#define whal_Stm32wb_AesEcb_Process whal_AesEcb_Process
#endif
#ifdef WHAL_CFG_STM32WB_AES_CBC_DIRECT_API_MAPPING
#define whal_Stm32wb_AesCbc_Oneshot whal_AesCbc_Oneshot
#define whal_Stm32wb_AesCbc_Start   whal_AesCbc_Start
#define whal_Stm32wb_AesCbc_Process whal_AesCbc_Process
#endif
#ifdef WHAL_CFG_STM32WB_AES_CTR_DIRECT_API_MAPPING
#define whal_Stm32wb_AesCtr_Oneshot whal_AesCtr_Oneshot
#define whal_Stm32wb_AesCtr_Start   whal_AesCtr_Start
#define whal_Stm32wb_AesCtr_Process whal_AesCtr_Process
#endif
#ifdef WHAL_CFG_STM32WB_AES_GCM_DIRECT_API_MAPPING
#define whal_Stm32wb_AesGcm_Oneshot  whal_AesGcm_Oneshot
#define whal_Stm32wb_AesGcm_Start    whal_AesGcm_Start
#define whal_Stm32wb_AesGcm_Process  whal_AesGcm_Process
#define whal_Stm32wb_AesGcm_Finalize whal_AesGcm_Finalize
#endif
#ifdef WHAL_CFG_STM32WB_AES_GMAC_DIRECT_API_MAPPING
#define whal_Stm32wb_AesGmac_Oneshot whal_AesGmac_Oneshot
#endif
#ifdef WHAL_CFG_STM32WB_AES_CCM_DIRECT_API_MAPPING
#define whal_Stm32wb_AesCcm_Oneshot  whal_AesCcm_Oneshot
#define whal_Stm32wb_AesCcm_Start    whal_AesCcm_Start
#define whal_Stm32wb_AesCcm_Process  whal_AesCcm_Process
#define whal_Stm32wb_AesCcm_Finalize whal_AesCcm_Finalize
#endif


/* ---- Init / Deinit ---- */

whal_Error whal_Stm32wb_Aes_Init(whal_Crypto *dev)
{
    (void)dev;
    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_Aes_Deinit(whal_Crypto *dev)
{
    (void)dev;
    DisableAes((whal_Crypto *)&whal_Stm32wb_Aes_Dev);
    ZeroKeyIv(whal_Stm32wb_Aes_Dev.base);
    return WHAL_SUCCESS;
}

const whal_CryptoDriver whal_Stm32wb_Aes_CryptoDriver = {
    .Init = whal_Stm32wb_Aes_Init,
    .Deinit = whal_Stm32wb_Aes_Deinit,
};


/* ---- AES-ECB ---- */

whal_Error whal_Stm32wb_AesEcb_Oneshot(whal_AesEcb *dev, whal_Crypto_Dir dir,
                              const void *key, size_t keySz,
                              const void *in, void *out, size_t sz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesEcb_Dev.crypto;
    size_t base = crypto->base;
    size_t mode;
    size_t keySizeBit;
    whal_Error err;

    (void)dev;

    if (!key)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    keySizeBit = (keySz == 32) ? 1 : 0;
    mode = (dir == WHAL_CRYPTO_ENCRYPT)
        ? AES_MODE_ENCRYPT : AES_MODE_KEYDERIV_DECRYPT;

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos, mode) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos,
                                 AES_CHMOD_ECB) |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit));

    WriteKey(base, key, keySz);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    err = ProcessBlockCipher(crypto, in, out, sz);
    DisableAes(crypto);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32wb_AesEcb_Start(whal_AesEcb *dev, whal_Crypto_Dir dir,
                            const void *key, size_t keySz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesEcb_Dev.crypto;
    size_t base = crypto->base;
    size_t mode;
    size_t keySizeBit;

    (void)dev;

    if (!key)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    keySizeBit = (keySz == 32) ? 1 : 0;
    mode = (dir == WHAL_CRYPTO_ENCRYPT)
        ? AES_MODE_ENCRYPT : AES_MODE_KEYDERIV_DECRYPT;

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos, mode) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos,
                                 AES_CHMOD_ECB) |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit));

    WriteKey(base, key, keySz);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_AesEcb_Process(whal_AesEcb *dev,
                              const void *in, void *out, size_t sz)
{
    (void)dev;
    return ProcessBlockCipher(whal_Stm32wb_AesEcb_Dev.crypto, in, out, sz);
}

const whal_AesEcbDriver whal_Stm32wb_Aes_EcbDriver = {
    .Oneshot = whal_Stm32wb_AesEcb_Oneshot,
    .Start = whal_Stm32wb_AesEcb_Start,
    .Process = whal_Stm32wb_AesEcb_Process,
};


/* ---- AES-CBC ---- */

whal_Error whal_Stm32wb_AesCbc_Oneshot(whal_AesCbc *dev, whal_Crypto_Dir dir,
                              const void *key, size_t keySz,
                              const void *iv,
                              const void *in, void *out, size_t sz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesCbc_Dev.crypto;
    size_t base = crypto->base;
    size_t mode;
    size_t keySizeBit;
    whal_Error err;

    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    keySizeBit = (keySz == 32) ? 1 : 0;
    mode = (dir == WHAL_CRYPTO_ENCRYPT)
        ? AES_MODE_ENCRYPT : AES_MODE_KEYDERIV_DECRYPT;

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos, mode) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos,
                                 AES_CHMOD_CBC) |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit));

    WriteKey(base, key, keySz);
    WriteIv(base, iv);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    err = ProcessBlockCipher(crypto, in, out, sz);
    DisableAes(crypto);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32wb_AesCbc_Start(whal_AesCbc *dev, whal_Crypto_Dir dir,
                            const void *key, size_t keySz,
                            const void *iv)
{
    whal_Crypto *crypto = whal_Stm32wb_AesCbc_Dev.crypto;
    size_t base = crypto->base;
    size_t mode;
    size_t keySizeBit;

    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    keySizeBit = (keySz == 32) ? 1 : 0;
    mode = (dir == WHAL_CRYPTO_ENCRYPT)
        ? AES_MODE_ENCRYPT : AES_MODE_KEYDERIV_DECRYPT;

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos, mode) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos,
                                 AES_CHMOD_CBC) |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit));

    WriteKey(base, key, keySz);
    WriteIv(base, iv);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_AesCbc_Process(whal_AesCbc *dev,
                              const void *in, void *out, size_t sz)
{
    (void)dev;
    return ProcessBlockCipher(whal_Stm32wb_AesCbc_Dev.crypto, in, out, sz);
}

const whal_AesCbcDriver whal_Stm32wb_Aes_CbcDriver = {
    .Oneshot = whal_Stm32wb_AesCbc_Oneshot,
    .Start = whal_Stm32wb_AesCbc_Start,
    .Process = whal_Stm32wb_AesCbc_Process,
};


/* ---- AES-CTR ---- */

whal_Error whal_Stm32wb_AesCtr_Oneshot(whal_AesCtr *dev, whal_Crypto_Dir dir,
                              const void *key, size_t keySz,
                              const void *iv,
                              const void *in, void *out, size_t sz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesCtr_Dev.crypto;
    size_t base = crypto->base;
    size_t keySizeBit;
    whal_Error err;

    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    keySizeBit = (keySz == 32) ? 1 : 0;

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos,
                                 AES_CHMOD_CTR) |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit));

    WriteKey(base, key, keySz);
    WriteIv(base, iv);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    err = ProcessBlockCipher(crypto, in, out, sz);
    DisableAes(crypto);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32wb_AesCtr_Start(whal_AesCtr *dev, whal_Crypto_Dir dir,
                            const void *key, size_t keySz,
                            const void *iv)
{
    whal_Crypto *crypto = whal_Stm32wb_AesCtr_Dev.crypto;
    size_t base = crypto->base;
    size_t keySizeBit;

    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    keySizeBit = (keySz == 32) ? 1 : 0;

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos,
                                 AES_CHMOD_CTR) |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit));

    WriteKey(base, key, keySz);
    WriteIv(base, iv);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_AesCtr_Process(whal_AesCtr *dev,
                              const void *in, void *out, size_t sz)
{
    (void)dev;
    return ProcessBlockCipher(whal_Stm32wb_AesCtr_Dev.crypto, in, out, sz);
}

const whal_AesCtrDriver whal_Stm32wb_Aes_CtrDriver = {
    .Oneshot = whal_Stm32wb_AesCtr_Oneshot,
    .Start = whal_Stm32wb_AesCtr_Start,
    .Process = whal_Stm32wb_AesCtr_Process,
};


/* ---- AES-GCM ---- */

whal_Error whal_Stm32wb_AesGcm_Oneshot(whal_AesGcm *dev, whal_Crypto_Dir dir,
                              const void *key, size_t keySz,
                              const void *iv, size_t ivSz,
                              const void *aad, size_t aadSz,
                              const void *in, void *out, size_t sz,
                              void *tag, size_t tagSz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesGcm_Dev.crypto;
    const whal_Stm32wb_Aes_Cfg *cfg =
        (const whal_Stm32wb_Aes_Cfg *)crypto->cfg;
    size_t base = crypto->base;
    const uint8_t *ivBytes = (const uint8_t *)iv;
    const uint8_t *aadBytes = (const uint8_t *)aad;
    size_t keySizeBit;
    size_t mode;
    size_t i;
    uint8_t tagBuf[16];
    whal_Error err;

    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    if (ivSz != 12)
        return WHAL_ENOTSUP;

    if (sz > 0 && (!in || !out))
        return WHAL_EINVAL;

    if (aadSz > 0 && !aad)
        return WHAL_EINVAL;

    if (!tag || tagSz == 0 || tagSz > 16)
        return WHAL_EINVAL;

    keySizeBit = (keySz == 32) ? 1 : 0;

    /* Init phase */
    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk |
                    AES_CR_GCMPH_Msk | AES_CR_NPBLB_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos,
                                 AES_CHMOD_GCM) |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_INIT));

    WriteKey(base, key, keySz);

    whal_Reg_Write(base, AES_IVR3_REG, whal_LoadBe32(ivBytes));
    whal_Reg_Write(base, AES_IVR2_REG, whal_LoadBe32(ivBytes + 4));
    whal_Reg_Write(base, AES_IVR1_REG, whal_LoadBe32(ivBytes + 8));
    whal_Reg_Write(base, AES_IVR0_REG, 0x00000002);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    err = WaitForCCF(base, cfg->timeout);
    if (err)
        goto cleanup;

    /* Header phase (AAD) */
    if (aadSz > 0) {
        whal_Reg_Update(base, AES_CR_REG,
                        AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                        whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                     AES_MODE_ENCRYPT) |
                        whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                     AES_GCMPH_HEADER));

        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

        for (i = 0; i < aadSz; i += 16) {
            const uint8_t *blkAad = aadBytes + i;
            size_t remain = aadSz - i;
            uint8_t block[16] = {0};
            size_t j;

            if (remain >= 16) {
                WriteBlock(base, blkAad);
            } else {
                for (j = 0; j < remain; j++)
                    block[j] = blkAad[j];
                WriteBlock(base, block);
            }

            err = WaitForCCF(base, cfg->timeout);
            if (err)
                goto cleanup;
        }
    }

    /* Payload phase */
    if (sz > 0) {
        mode = (dir == WHAL_CRYPTO_ENCRYPT)
            ? AES_MODE_ENCRYPT : AES_MODE_DECRYPT;

        whal_Reg_Update(base, AES_CR_REG,
                        AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                        whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                     mode) |
                        whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                     AES_GCMPH_PAYLOAD));

        if (aadSz == 0)
            whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

        for (i = 0; i < sz; i += 16) {
            const uint8_t *inBlk = (const uint8_t *)in + i;
            uint8_t *outBlk = (uint8_t *)out + i;
            size_t remain = sz - i;
            uint8_t block[16] = {0};
            size_t j;

            if (remain >= 16) {
                WriteBlock(base, inBlk);
            } else {
                if (mode == AES_MODE_ENCRYPT) {
                    whal_Reg_Update(base, AES_CR_REG, AES_CR_NPBLB_Msk,
                                    whal_SetBits(AES_CR_NPBLB_Msk,
                                                 AES_CR_NPBLB_Pos,
                                                 16 - remain));
                }
                for (j = 0; j < remain; j++)
                    block[j] = inBlk[j];
                WriteBlock(base, block);
            }

            err = WaitForCCF(base, cfg->timeout);
            if (err)
                goto cleanup;

            if (remain >= 16) {
                ReadBlock(base, outBlk);
            } else {
                ReadBlock(base, block);
                for (j = 0; j < remain; j++)
                    outBlk[j] = block[j];
            }
        }
    }

    /* Final phase (tag) */
    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_FINAL));

    if (aadSz == 0 && sz == 0)
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, (uint32_t)(aadSz * 8));
    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, (uint32_t)(sz * 8));

    err = WaitForCCF(base, cfg->timeout);
    if (err)
        goto cleanup;

    ReadBlock(base, tagBuf);

    for (i = 0; i < tagSz; i++)
        ((uint8_t *)tag)[i] = tagBuf[i];

cleanup:
    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32wb_AesGcm_Start(whal_AesGcm *dev, whal_Crypto_Dir dir,
                            const void *key, size_t keySz,
                            const void *iv, size_t ivSz,
                            const void *aad, size_t aadSz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesGcm_Dev.crypto;
    const whal_Stm32wb_Aes_Cfg *cfg =
        (const whal_Stm32wb_Aes_Cfg *)crypto->cfg;
    size_t base = crypto->base;
    const uint8_t *ivBytes = (const uint8_t *)iv;
    const uint8_t *aadBytes = (const uint8_t *)aad;
    size_t keySizeBit;
    size_t mode;
    size_t i;
    whal_Error err;

    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    if (ivSz != 12)
        return WHAL_ENOTSUP;

    if (aadSz > 0 && !aad)
        return WHAL_EINVAL;

    keySizeBit = (keySz == 32) ? 1 : 0;

    /* Init phase */
    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk |
                    AES_CR_GCMPH_Msk | AES_CR_NPBLB_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos,
                                 AES_CHMOD_GCM) |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_INIT));

    WriteKey(base, key, keySz);

    whal_Reg_Write(base, AES_IVR3_REG, whal_LoadBe32(ivBytes));
    whal_Reg_Write(base, AES_IVR2_REG, whal_LoadBe32(ivBytes + 4));
    whal_Reg_Write(base, AES_IVR1_REG, whal_LoadBe32(ivBytes + 8));
    whal_Reg_Write(base, AES_IVR0_REG, 0x00000002);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    err = WaitForCCF(base, cfg->timeout);
    if (err)
        goto cleanup;

    /* Header phase (AAD) */
    if (aadSz > 0) {
        whal_Reg_Update(base, AES_CR_REG,
                        AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                        whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                     AES_MODE_ENCRYPT) |
                        whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                     AES_GCMPH_HEADER));

        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

        for (i = 0; i < aadSz; i += 16) {
            const uint8_t *blkAad = aadBytes + i;
            size_t remain = aadSz - i;
            uint8_t block[16] = {0};
            size_t j;

            if (remain >= 16) {
                WriteBlock(base, blkAad);
            } else {
                for (j = 0; j < remain; j++)
                    block[j] = blkAad[j];
                WriteBlock(base, block);
            }

            err = WaitForCCF(base, cfg->timeout);
            if (err)
                goto cleanup;
        }
    }

    /* Transition to payload phase */
    mode = (dir == WHAL_CRYPTO_ENCRYPT)
        ? AES_MODE_ENCRYPT : AES_MODE_DECRYPT;

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos, mode) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_PAYLOAD));

    if (aadSz == 0)
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    {
        whal_Stm32wb_AesGcm_State *st =
            (whal_Stm32wb_AesGcm_State *)whal_Stm32wb_AesGcm_Dev.state;
        st->aadSz = aadSz;
        st->dataSz = 0;
    }

    return WHAL_SUCCESS;

cleanup:
    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32wb_AesGcm_Process(whal_AesGcm *dev,
                              const void *in, void *out, size_t sz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesGcm_Dev.crypto;
    const whal_Stm32wb_Aes_Cfg *cfg =
        (const whal_Stm32wb_Aes_Cfg *)crypto->cfg;
    size_t base = crypto->base;
    whal_Stm32wb_AesGcm_State *st =
        (whal_Stm32wb_AesGcm_State *)whal_Stm32wb_AesGcm_Dev.state;
    size_t mode;
    size_t i;
    whal_Error err;

    (void)dev;

    if (sz == 0)
        return WHAL_SUCCESS;

    if (!in || !out) {
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
        ZeroKeyIv(base);
        return WHAL_EINVAL;
    }

    mode = whal_GetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                        whal_Reg_Read(base, AES_CR_REG));

    for (i = 0; i < sz; i += 16) {
        const uint8_t *inBlk = (const uint8_t *)in + i;
        uint8_t *outBlk = (uint8_t *)out + i;
        size_t remain = sz - i;
        uint8_t block[16] = {0};
        size_t j;

        if (remain >= 16) {
            WriteBlock(base, inBlk);
        } else {
            if (mode == AES_MODE_ENCRYPT) {
                whal_Reg_Update(base, AES_CR_REG, AES_CR_NPBLB_Msk,
                                whal_SetBits(AES_CR_NPBLB_Msk,
                                             AES_CR_NPBLB_Pos,
                                             16 - remain));
            }
            for (j = 0; j < remain; j++)
                block[j] = inBlk[j];
            WriteBlock(base, block);
        }

        err = WaitForCCF(base, cfg->timeout);
        if (err) {
            whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
            ZeroKeyIv(base);
            return err;
        }

        if (remain >= 16) {
            ReadBlock(base, outBlk);
        } else {
            ReadBlock(base, block);
            for (j = 0; j < remain; j++)
                outBlk[j] = block[j];
        }
    }

    st->dataSz += sz;

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_AesGcm_Finalize(whal_AesGcm *dev,
                               void *tag, size_t tagSz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesGcm_Dev.crypto;
    const whal_Stm32wb_Aes_Cfg *cfg =
        (const whal_Stm32wb_Aes_Cfg *)crypto->cfg;
    size_t base = crypto->base;
    whal_Stm32wb_AesGcm_State *st =
        (whal_Stm32wb_AesGcm_State *)whal_Stm32wb_AesGcm_Dev.state;
    uint8_t tagBuf[16];
    size_t i;
    whal_Error err;

    (void)dev;

    if (!tag || tagSz == 0 || tagSz > 16) {
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
        ZeroKeyIv(base);
        return WHAL_EINVAL;
    }

    /* Final phase (tag) */
    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_FINAL));

    if (st->aadSz == 0 && st->dataSz == 0)
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, (uint32_t)(st->aadSz * 8));
    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, (uint32_t)(st->dataSz * 8));

    err = WaitForCCF(base, cfg->timeout);
    if (err) {
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
        ZeroKeyIv(base);
        return err;
    }

    ReadBlock(base, tagBuf);

    for (i = 0; i < tagSz; i++)
        ((uint8_t *)tag)[i] = tagBuf[i];

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
    ZeroKeyIv(base);
    return WHAL_SUCCESS;
}

const whal_AesGcmDriver whal_Stm32wb_Aes_GcmDriver = {
    .Oneshot = whal_Stm32wb_AesGcm_Oneshot,
    .Start = whal_Stm32wb_AesGcm_Start,
    .Process = whal_Stm32wb_AesGcm_Process,
    .Finalize = whal_Stm32wb_AesGcm_Finalize,
};


/* ---- AES-GMAC ---- */

whal_Error whal_Stm32wb_AesGmac_Oneshot(whal_AesGmac *dev,
                               const void *key, size_t keySz,
                               const void *iv, size_t ivSz,
                               const void *aad, size_t aadSz,
                               void *tag, size_t tagSz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesGmac_Dev.crypto;
    const whal_Stm32wb_Aes_Cfg *cfg =
        (const whal_Stm32wb_Aes_Cfg *)crypto->cfg;
    size_t base = crypto->base;
    const uint8_t *ivBytes = (const uint8_t *)iv;
    const uint8_t *aadBytes = (const uint8_t *)aad;
    size_t keySizeBit;
    size_t i;
    uint8_t tagBuf[16];
    whal_Error err;

    (void)dev;

    if (!key || !iv)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    if (ivSz != 12)
        return WHAL_ENOTSUP;

    if (aadSz > 0 && !aad)
        return WHAL_EINVAL;

    if (!tag || tagSz == 0 || tagSz > 16)
        return WHAL_EINVAL;

    keySizeBit = (keySz == 32) ? 1 : 0;

    /* Init phase */
    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk |
                    AES_CR_GCMPH_Msk | AES_CR_NPBLB_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos,
                                 AES_CHMOD_GCM) |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_INIT));

    WriteKey(base, key, keySz);

    whal_Reg_Write(base, AES_IVR3_REG, whal_LoadBe32(ivBytes));
    whal_Reg_Write(base, AES_IVR2_REG, whal_LoadBe32(ivBytes + 4));
    whal_Reg_Write(base, AES_IVR1_REG, whal_LoadBe32(ivBytes + 8));
    whal_Reg_Write(base, AES_IVR0_REG, 0x00000002);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    err = WaitForCCF(base, cfg->timeout);
    if (err)
        goto cleanup;

    /* Header phase (AAD) */
    if (aadSz > 0) {
        whal_Reg_Update(base, AES_CR_REG,
                        AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                        whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                     AES_MODE_ENCRYPT) |
                        whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                     AES_GCMPH_HEADER));

        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

        for (i = 0; i < aadSz; i += 16) {
            const uint8_t *blkAad = aadBytes + i;
            size_t remain = aadSz - i;
            uint8_t block[16] = {0};
            size_t j;

            if (remain >= 16) {
                WriteBlock(base, blkAad);
            } else {
                for (j = 0; j < remain; j++)
                    block[j] = blkAad[j];
                WriteBlock(base, block);
            }

            err = WaitForCCF(base, cfg->timeout);
            if (err)
                goto cleanup;
        }
    }

    /* Final phase (tag) -- no payload for GMAC */
    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_FINAL));

    if (aadSz == 0)
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, (uint32_t)(aadSz * 8));
    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, 0);

    err = WaitForCCF(base, cfg->timeout);
    if (err)
        goto cleanup;

    ReadBlock(base, tagBuf);

    for (i = 0; i < tagSz; i++)
        ((uint8_t *)tag)[i] = tagBuf[i];

cleanup:
    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
    ZeroKeyIv(base);
    return err;
}

const whal_AesGmacDriver whal_Stm32wb_Aes_GmacDriver = {
    .Oneshot = whal_Stm32wb_AesGmac_Oneshot,
};


/* ---- AES-CCM ---- */

whal_Error whal_Stm32wb_AesCcm_Oneshot(whal_AesCcm *dev, whal_Crypto_Dir dir,
                              const void *key, size_t keySz,
                              const void *nonce, size_t nonceSz,
                              const void *aad, size_t aadSz,
                              const void *in, void *out, size_t sz,
                              void *tag, size_t tagSz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesCcm_Dev.crypto;
    const whal_Stm32wb_Aes_Cfg *cfg =
        (const whal_Stm32wb_Aes_Cfg *)crypto->cfg;
    size_t base = crypto->base;
    const uint8_t *nonceBytes = (const uint8_t *)nonce;
    const uint8_t *aadBytes = (const uint8_t *)aad;
    size_t keySizeBit;
    size_t mode;
    size_t i;
    uint8_t b0[16];
    uint8_t tagBuf[16];
    whal_Error err;

    (void)dev;

    if (!key || !nonce)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    if (nonceSz < 7 || nonceSz > 13)
        return WHAL_EINVAL;

    if (sz > 0 && (!in || !out))
        return WHAL_EINVAL;

    if (aadSz > 0 && !aad)
        return WHAL_EINVAL;

    if (!tag || tagSz < 4 || tagSz > 16 || (tagSz & 1) != 0)
        return WHAL_EINVAL;

    keySizeBit = (keySz == 32) ? 1 : 0;

    /* Build B0 */
    {
        size_t q = 15 - nonceSz;
        size_t t = tagSz;
        uint8_t flags = (uint8_t)(((aadSz > 0) ? 0x40 : 0) |
                                  (((t - 2) / 2) << 3) |
                                  (q - 1));
        b0[0] = flags;
        for (i = 0; i < nonceSz; i++)
            b0[1 + i] = nonceBytes[i];

        {
            size_t msgLen = sz;
            size_t j;
            for (j = 0; j < q; j++) {
                b0[15 - j] = (uint8_t)(msgLen & 0xFF);
                msgLen >>= 8;
            }
        }
    }

    /* Init phase */
    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk |
                    AES_CR_GCMPH_Msk | AES_CR_NPBLB_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos, 0) |
                    AES_CR_CHMOD2_Msk |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_INIT));

    WriteKey(base, key, keySz);
    WriteIv(base, b0);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    err = WaitForCCF(base, cfg->timeout);
    if (err)
        goto cleanup;

    /* Header phase (AAD) */
    if (aadSz > 0) {
        whal_Reg_Update(base, AES_CR_REG,
                        AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                        whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                     AES_MODE_ENCRYPT) |
                        whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                     AES_GCMPH_HEADER));

        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

        {
            uint8_t hdrBuf[16] = {0};
            size_t hdrOff = 0;
            size_t aadOff = 0;
            size_t j;

            hdrBuf[0] = (uint8_t)(aadSz >> 8);
            hdrBuf[1] = (uint8_t)(aadSz);
            hdrOff = 2;

            while (hdrOff < 16 && aadOff < aadSz) {
                hdrBuf[hdrOff++] = aadBytes[aadOff++];
            }

            WriteBlock(base, hdrBuf);
            err = WaitForCCF(base, cfg->timeout);
            if (err)
                goto cleanup;

            while (aadOff < aadSz) {
                uint8_t block[16] = {0};

                for (j = 0; j < 16 && aadOff < aadSz; j++)
                    block[j] = aadBytes[aadOff++];

                WriteBlock(base, block);
                err = WaitForCCF(base, cfg->timeout);
                if (err)
                    goto cleanup;
            }
        }
    }

    /* Payload phase */
    if (sz > 0) {
        mode = (dir == WHAL_CRYPTO_ENCRYPT)
            ? AES_MODE_ENCRYPT : AES_MODE_DECRYPT;

        whal_Reg_Update(base, AES_CR_REG,
                        AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                        whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                     mode) |
                        whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                     AES_GCMPH_PAYLOAD));

        if (aadSz == 0)
            whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

        for (i = 0; i < sz; i += 16) {
            const uint8_t *inBlk = (const uint8_t *)in + i;
            uint8_t *outBlk = (uint8_t *)out + i;
            size_t remain = sz - i;
            uint8_t block[16] = {0};
            size_t j;

            if (remain >= 16) {
                WriteBlock(base, inBlk);
            } else {
                if (mode == AES_MODE_DECRYPT) {
                    whal_Reg_Update(base, AES_CR_REG, AES_CR_NPBLB_Msk,
                                    whal_SetBits(AES_CR_NPBLB_Msk,
                                                 AES_CR_NPBLB_Pos,
                                                 16 - remain));
                }
                for (j = 0; j < remain; j++)
                    block[j] = inBlk[j];
                WriteBlock(base, block);
            }

            err = WaitForCCF(base, cfg->timeout);
            if (err)
                goto cleanup;

            if (remain >= 16) {
                ReadBlock(base, outBlk);
            } else {
                ReadBlock(base, block);
                for (j = 0; j < remain; j++)
                    outBlk[j] = block[j];
            }
        }
    }

    /* Final phase (tag) */
    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_FINAL));

    if (aadSz == 0 && sz == 0)
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, 0);

    err = WaitForCCF(base, cfg->timeout);
    if (err)
        goto cleanup;

    ReadBlock(base, tagBuf);

    for (i = 0; i < tagSz; i++)
        ((uint8_t *)tag)[i] = tagBuf[i];

cleanup:
    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32wb_AesCcm_Start(whal_AesCcm *dev, whal_Crypto_Dir dir,
                            const void *key, size_t keySz,
                            const void *nonce, size_t nonceSz,
                            const void *aad, size_t aadSz,
                            size_t tagSz, size_t sz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesCcm_Dev.crypto;
    const whal_Stm32wb_Aes_Cfg *cfg =
        (const whal_Stm32wb_Aes_Cfg *)crypto->cfg;
    size_t base = crypto->base;
    const uint8_t *nonceBytes = (const uint8_t *)nonce;
    const uint8_t *aadBytes = (const uint8_t *)aad;
    size_t keySizeBit;
    size_t mode;
    size_t i;
    uint8_t b0[16];
    whal_Error err;

    (void)dev;

    if (!key || !nonce)
        return WHAL_EINVAL;

    if (keySz != 16 && keySz != 32)
        return WHAL_ENOTSUP;

    if (nonceSz < 7 || nonceSz > 13)
        return WHAL_EINVAL;

    if (aadSz > 0 && !aad)
        return WHAL_EINVAL;

    if (tagSz < 4 || tagSz > 16 || (tagSz & 1) != 0)
        return WHAL_EINVAL;

    keySizeBit = (keySz == 32) ? 1 : 0;

    /* Build B0 */
    {
        size_t q = 15 - nonceSz;
        size_t t = tagSz;
        uint8_t flags = (uint8_t)(((aadSz > 0) ? 0x40 : 0) |
                                  (((t - 2) / 2) << 3) |
                                  (q - 1));
        b0[0] = flags;
        for (i = 0; i < nonceSz; i++)
            b0[1 + i] = nonceBytes[i];

        {
            size_t msgLen = sz;
            size_t j;
            for (j = 0; j < q; j++) {
                b0[15 - j] = (uint8_t)(msgLen & 0xFF);
                msgLen >>= 8;
            }
        }
    }

    /* Init phase */
    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_CHMOD_Msk | AES_CR_CHMOD2_Msk |
                    AES_CR_DATATYPE_Msk | AES_CR_KEYSIZE_Msk |
                    AES_CR_GCMPH_Msk | AES_CR_NPBLB_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_CHMOD_Msk, AES_CR_CHMOD_Pos, 0) |
                    AES_CR_CHMOD2_Msk |
                    whal_SetBits(AES_CR_DATATYPE_Msk, AES_CR_DATATYPE_Pos,
                                 AES_DATATYPE_NONE) |
                    whal_SetBits(AES_CR_KEYSIZE_Msk, AES_CR_KEYSIZE_Pos,
                                 keySizeBit) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_INIT));

    WriteKey(base, key, keySz);
    WriteIv(base, b0);

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    err = WaitForCCF(base, cfg->timeout);
    if (err)
        goto cleanup;

    /* Header phase (AAD) */
    if (aadSz > 0) {
        whal_Reg_Update(base, AES_CR_REG,
                        AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                        whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                     AES_MODE_ENCRYPT) |
                        whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                     AES_GCMPH_HEADER));

        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

        {
            uint8_t hdrBuf[16] = {0};
            size_t hdrOff = 0;
            size_t aadOff = 0;
            size_t j;

            hdrBuf[0] = (uint8_t)(aadSz >> 8);
            hdrBuf[1] = (uint8_t)(aadSz);
            hdrOff = 2;

            while (hdrOff < 16 && aadOff < aadSz) {
                hdrBuf[hdrOff++] = aadBytes[aadOff++];
            }

            WriteBlock(base, hdrBuf);
            err = WaitForCCF(base, cfg->timeout);
            if (err)
                goto cleanup;

            while (aadOff < aadSz) {
                uint8_t block[16] = {0};

                for (j = 0; j < 16 && aadOff < aadSz; j++)
                    block[j] = aadBytes[aadOff++];

                WriteBlock(base, block);
                err = WaitForCCF(base, cfg->timeout);
                if (err)
                    goto cleanup;
            }
        }
    }

    /* Transition to payload phase */
    mode = (dir == WHAL_CRYPTO_ENCRYPT)
        ? AES_MODE_ENCRYPT : AES_MODE_DECRYPT;

    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos, mode) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_PAYLOAD));

    if (aadSz == 0)
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    {
        whal_Stm32wb_AesCcm_State *st =
            (whal_Stm32wb_AesCcm_State *)whal_Stm32wb_AesCcm_Dev.state;
        st->aadSz = aadSz;
        st->dataSz = 0;
    }

    return WHAL_SUCCESS;

cleanup:
    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
    ZeroKeyIv(base);
    return err;
}

whal_Error whal_Stm32wb_AesCcm_Process(whal_AesCcm *dev,
                              const void *in, void *out, size_t sz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesCcm_Dev.crypto;
    const whal_Stm32wb_Aes_Cfg *cfg =
        (const whal_Stm32wb_Aes_Cfg *)crypto->cfg;
    size_t base = crypto->base;
    whal_Stm32wb_AesCcm_State *st =
        (whal_Stm32wb_AesCcm_State *)whal_Stm32wb_AesCcm_Dev.state;
    size_t mode;
    size_t i;
    whal_Error err;

    (void)dev;

    if (sz == 0)
        return WHAL_SUCCESS;

    if (!in || !out) {
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
        ZeroKeyIv(base);
        return WHAL_EINVAL;
    }

    mode = whal_GetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                        whal_Reg_Read(base, AES_CR_REG));

    for (i = 0; i < sz; i += 16) {
        const uint8_t *inBlk = (const uint8_t *)in + i;
        uint8_t *outBlk = (uint8_t *)out + i;
        size_t remain = sz - i;
        uint8_t block[16] = {0};
        size_t j;

        if (remain >= 16) {
            WriteBlock(base, inBlk);
        } else {
            if (mode == AES_MODE_DECRYPT) {
                whal_Reg_Update(base, AES_CR_REG, AES_CR_NPBLB_Msk,
                                whal_SetBits(AES_CR_NPBLB_Msk,
                                             AES_CR_NPBLB_Pos,
                                             16 - remain));
            }
            for (j = 0; j < remain; j++)
                block[j] = inBlk[j];
            WriteBlock(base, block);
        }

        err = WaitForCCF(base, cfg->timeout);
        if (err) {
            whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
            ZeroKeyIv(base);
            return err;
        }

        if (remain >= 16) {
            ReadBlock(base, outBlk);
        } else {
            ReadBlock(base, block);
            for (j = 0; j < remain; j++)
                outBlk[j] = block[j];
        }
    }

    st->dataSz += sz;

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wb_AesCcm_Finalize(whal_AesCcm *dev,
                               void *tag, size_t tagSz)
{
    whal_Crypto *crypto = whal_Stm32wb_AesCcm_Dev.crypto;
    const whal_Stm32wb_Aes_Cfg *cfg =
        (const whal_Stm32wb_Aes_Cfg *)crypto->cfg;
    size_t base = crypto->base;
    whal_Stm32wb_AesCcm_State *st =
        (whal_Stm32wb_AesCcm_State *)whal_Stm32wb_AesCcm_Dev.state;
    uint8_t tagBuf[16];
    size_t i;
    whal_Error err;

    (void)dev;

    if (!tag || tagSz < 4 || tagSz > 16 || (tagSz & 1) != 0) {
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
        ZeroKeyIv(base);
        return WHAL_EINVAL;
    }

    /* Final phase (tag) */
    whal_Reg_Update(base, AES_CR_REG,
                    AES_CR_MODE_Msk | AES_CR_GCMPH_Msk,
                    whal_SetBits(AES_CR_MODE_Msk, AES_CR_MODE_Pos,
                                 AES_MODE_ENCRYPT) |
                    whal_SetBits(AES_CR_GCMPH_Msk, AES_CR_GCMPH_Pos,
                                 AES_GCMPH_FINAL));

    if (st->aadSz == 0 && st->dataSz == 0)
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, AES_CR_EN_Msk);

    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, 0);
    whal_Reg_Write(base, AES_DINR_REG, 0);

    err = WaitForCCF(base, cfg->timeout);
    if (err) {
        whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
        ZeroKeyIv(base);
        return err;
    }

    ReadBlock(base, tagBuf);

    for (i = 0; i < tagSz; i++)
        ((uint8_t *)tag)[i] = tagBuf[i];

    whal_Reg_Update(base, AES_CR_REG, AES_CR_EN_Msk, 0);
    ZeroKeyIv(base);
    return WHAL_SUCCESS;
}

const whal_AesCcmDriver whal_Stm32wb_Aes_CcmDriver = {
    .Oneshot = whal_Stm32wb_AesCcm_Oneshot,
    .Start = whal_Stm32wb_AesCcm_Start,
    .Process = whal_Stm32wb_AesCcm_Process,
    .Finalize = whal_Stm32wb_AesCcm_Finalize,
};
