/* stm32wb_aes.h
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

#ifndef WHAL_STM32WB_AES_H
#define WHAL_STM32WB_AES_H

#include <wolfHAL/crypto/crypto.h>
#include <wolfHAL/timeout.h>

/**
 * @file stm32wb_aes.h
 * @brief STM32WB AES hardware accelerator driver.
 *
 * The STM32WB AES1 peripheral supports 128/256-bit keys in ECB, CBC,
 * CTR, GCM, GMAC, and CCM modes. Each algorithm is exposed through
 * a per-algorithm vtable with Oneshot/Start/Process/Finalize operations.
 */

/**
 * @brief AES device configuration.
 */
typedef struct {
    whal_Timeout *timeout;
} whal_Stm32wb_Aes_Cfg;

/* ---- Streaming state ---- */

/**
 * @brief AES-GCM streaming state (aadSz/dataSz for final-phase GHASH).
 */
typedef struct {
    size_t aadSz;
    size_t dataSz;
} whal_Stm32wb_AesGcm_State;

/**
 * @brief AES-CCM streaming state (aadSz/dataSz for final-phase tag).
 */
typedef struct {
    size_t aadSz;
    size_t dataSz;
} whal_Stm32wb_AesCcm_State;

/* ---- Direct API mapping ---- */

#ifdef WHAL_CFG_STM32WB_AES_INIT_DIRECT_API_MAPPING
#define whal_Stm32wb_Aes_Init        whal_Crypto_Init
#define whal_Stm32wb_Aes_Deinit      whal_Crypto_Deinit
#endif

#ifdef WHAL_CFG_STM32WB_AES_ECB_DIRECT_API_MAPPING
#define whal_Stm32wb_AesEcb_Oneshot  whal_AesEcb_Oneshot
#define whal_Stm32wb_AesEcb_Start    whal_AesEcb_Start
#define whal_Stm32wb_AesEcb_Process  whal_AesEcb_Process
#endif

#ifdef WHAL_CFG_STM32WB_AES_CBC_DIRECT_API_MAPPING
#define whal_Stm32wb_AesCbc_Oneshot  whal_AesCbc_Oneshot
#define whal_Stm32wb_AesCbc_Start    whal_AesCbc_Start
#define whal_Stm32wb_AesCbc_Process  whal_AesCbc_Process
#endif

#ifdef WHAL_CFG_STM32WB_AES_CTR_DIRECT_API_MAPPING
#define whal_Stm32wb_AesCtr_Oneshot  whal_AesCtr_Oneshot
#define whal_Stm32wb_AesCtr_Start    whal_AesCtr_Start
#define whal_Stm32wb_AesCtr_Process  whal_AesCtr_Process
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

/**
 * @brief Initialize the STM32WB AES peripheral.
 *
 * @param dev Crypto device instance.
 */
whal_Error whal_Stm32wb_Aes_Init(whal_Crypto *dev);

/**
 * @brief Deinitialize the STM32WB AES peripheral.
 *
 * @param dev Crypto device instance.
 */
whal_Error whal_Stm32wb_Aes_Deinit(whal_Crypto *dev);

/* ---- AES-ECB ---- */

/**
 * @brief AES-ECB one-shot encrypt or decrypt.
 *
 * @param dev   AES-ECB device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes (16 or 32).
 * @param in    Input data (multiple of 16 bytes).
 * @param out   Output buffer.
 * @param sz    Data size in bytes.
 */
whal_Error whal_Stm32wb_AesEcb_Oneshot(whal_AesEcb *dev, whal_Crypto_Dir dir,
                                       const void *key, size_t keySz,
                                       const void *in, void *out, size_t sz);

/**
 * @brief Start an AES-ECB streaming session (load key).
 *
 * @param dev   AES-ECB device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes (16 or 32).
 */
whal_Error whal_Stm32wb_AesEcb_Start(whal_AesEcb *dev, whal_Crypto_Dir dir,
                                     const void *key, size_t keySz);

/**
 * @brief Process data through an active AES-ECB session.
 *
 * @param dev AES-ECB device instance.
 * @param in  Input data (multiple of 16 bytes).
 * @param out Output buffer.
 * @param sz  Data size in bytes.
 */
whal_Error whal_Stm32wb_AesEcb_Process(whal_AesEcb *dev,
                                       const void *in, void *out, size_t sz);

/* ---- AES-CBC ---- */

/**
 * @brief AES-CBC one-shot encrypt or decrypt.
 *
 * @param dev   AES-CBC device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes (16 or 32).
 * @param iv    Initialization vector (16 bytes).
 * @param in    Input data (multiple of 16 bytes).
 * @param out   Output buffer.
 * @param sz    Data size in bytes.
 */
whal_Error whal_Stm32wb_AesCbc_Oneshot(whal_AesCbc *dev, whal_Crypto_Dir dir,
                                       const void *key, size_t keySz,
                                       const void *iv,
                                       const void *in, void *out, size_t sz);

/**
 * @brief Start an AES-CBC streaming session (load key and IV).
 *
 * @param dev   AES-CBC device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes (16 or 32).
 * @param iv    Initialization vector (16 bytes).
 */
whal_Error whal_Stm32wb_AesCbc_Start(whal_AesCbc *dev, whal_Crypto_Dir dir,
                                     const void *key, size_t keySz,
                                     const void *iv);

/**
 * @brief Process data through an active AES-CBC session.
 *
 * @param dev AES-CBC device instance.
 * @param in  Input data (multiple of 16 bytes).
 * @param out Output buffer.
 * @param sz  Data size in bytes.
 */
whal_Error whal_Stm32wb_AesCbc_Process(whal_AesCbc *dev,
                                       const void *in, void *out, size_t sz);

/* ---- AES-CTR ---- */

/**
 * @brief AES-CTR one-shot encrypt or decrypt.
 *
 * @param dev   AES-CTR device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes (16 or 32).
 * @param iv    Initial counter block (16 bytes).
 * @param in    Input data.
 * @param out   Output buffer.
 * @param sz    Data size in bytes.
 */
whal_Error whal_Stm32wb_AesCtr_Oneshot(whal_AesCtr *dev, whal_Crypto_Dir dir,
                                       const void *key, size_t keySz,
                                       const void *iv,
                                       const void *in, void *out, size_t sz);

/**
 * @brief Start an AES-CTR streaming session (load key and counter).
 *
 * @param dev   AES-CTR device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes (16 or 32).
 * @param iv    Initial counter block (16 bytes).
 */
whal_Error whal_Stm32wb_AesCtr_Start(whal_AesCtr *dev, whal_Crypto_Dir dir,
                                     const void *key, size_t keySz,
                                     const void *iv);

/**
 * @brief Process data through an active AES-CTR session.
 *
 * @param dev AES-CTR device instance.
 * @param in  Input data.
 * @param out Output buffer.
 * @param sz  Data size in bytes.
 */
whal_Error whal_Stm32wb_AesCtr_Process(whal_AesCtr *dev,
                                       const void *in, void *out, size_t sz);

/* ---- AES-GCM ---- */

/**
 * @brief AES-GCM one-shot authenticated encrypt or decrypt.
 *
 * @param dev   AES-GCM device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes (16 or 32).
 * @param iv    Initialization vector.
 * @param ivSz  IV size in bytes (typically 12).
 * @param aad   Additional authenticated data.
 * @param aadSz AAD size in bytes.
 * @param in    Input data.
 * @param out   Output buffer.
 * @param sz    Data size in bytes.
 * @param tag   Authentication tag output.
 * @param tagSz Tag size in bytes (up to 16).
 */
whal_Error whal_Stm32wb_AesGcm_Oneshot(whal_AesGcm *dev, whal_Crypto_Dir dir,
                                       const void *key, size_t keySz,
                                       const void *iv, size_t ivSz,
                                       const void *aad, size_t aadSz,
                                       const void *in, void *out, size_t sz,
                                       void *tag, size_t tagSz);

/**
 * @brief Start an AES-GCM streaming session (init + header phases).
 *
 * @param dev   AES-GCM device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes (16 or 32).
 * @param iv    Initialization vector.
 * @param ivSz  IV size in bytes (typically 12).
 * @param aad   Additional authenticated data.
 * @param aadSz AAD size in bytes.
 */
whal_Error whal_Stm32wb_AesGcm_Start(whal_AesGcm *dev, whal_Crypto_Dir dir,
                                     const void *key, size_t keySz,
                                     const void *iv, size_t ivSz,
                                     const void *aad, size_t aadSz);

/**
 * @brief Process payload data through an active AES-GCM session.
 *
 * @param dev AES-GCM device instance.
 * @param in  Input data.
 * @param out Output buffer.
 * @param sz  Data size in bytes.
 */
whal_Error whal_Stm32wb_AesGcm_Process(whal_AesGcm *dev,
                                       const void *in, void *out, size_t sz);

/**
 * @brief Finalize an AES-GCM session and produce the authentication tag.
 *
 * @param dev   AES-GCM device instance.
 * @param tag   Authentication tag output.
 * @param tagSz Tag size in bytes (up to 16).
 */
whal_Error whal_Stm32wb_AesGcm_Finalize(whal_AesGcm *dev,
                                        void *tag, size_t tagSz);

/* ---- AES-GMAC ---- */

/**
 * @brief AES-GMAC one-shot authentication (no payload).
 *
 * @param dev   AES-GMAC device instance.
 * @param key   Key buffer.
 * @param keySz Key size in bytes (16 or 32).
 * @param iv    Initialization vector.
 * @param ivSz  IV size in bytes (typically 12).
 * @param aad   Authenticated data.
 * @param aadSz Data size in bytes.
 * @param tag   Authentication tag output.
 * @param tagSz Tag size in bytes (up to 16).
 */
whal_Error whal_Stm32wb_AesGmac_Oneshot(whal_AesGmac *dev,
                                        const void *key, size_t keySz,
                                        const void *iv, size_t ivSz,
                                        const void *aad, size_t aadSz,
                                        void *tag, size_t tagSz);

/* ---- AES-CCM ---- */

/**
 * @brief AES-CCM one-shot authenticated encrypt or decrypt.
 *
 * @param dev     AES-CCM device instance.
 * @param dir     Encrypt or decrypt.
 * @param key     Key buffer.
 * @param keySz   Key size in bytes (16 or 32).
 * @param nonce   Nonce buffer.
 * @param nonceSz Nonce size in bytes (7-13).
 * @param aad     Additional authenticated data.
 * @param aadSz   AAD size in bytes.
 * @param in      Input data.
 * @param out     Output buffer.
 * @param sz      Data size in bytes.
 * @param tag     Authentication tag output.
 * @param tagSz   Tag size in bytes (4, 6, 8, 10, 12, 14, or 16).
 */
whal_Error whal_Stm32wb_AesCcm_Oneshot(whal_AesCcm *dev, whal_Crypto_Dir dir,
                                       const void *key, size_t keySz,
                                       const void *nonce, size_t nonceSz,
                                       const void *aad, size_t aadSz,
                                       const void *in, void *out, size_t sz,
                                       void *tag, size_t tagSz);

/**
 * @brief Start an AES-CCM streaming session (init + header phases).
 *
 * @param dev     AES-CCM device instance.
 * @param dir     Encrypt or decrypt.
 * @param key     Key buffer.
 * @param keySz   Key size in bytes (16 or 32).
 * @param nonce   Nonce buffer.
 * @param nonceSz Nonce size in bytes (7-13).
 * @param aad     Additional authenticated data.
 * @param aadSz   AAD size in bytes.
 * @param tagSz   Tag size (needed for B0 block construction).
 * @param sz      Total payload size (needed for B0 block construction).
 */
whal_Error whal_Stm32wb_AesCcm_Start(whal_AesCcm *dev, whal_Crypto_Dir dir,
                                     const void *key, size_t keySz,
                                     const void *nonce, size_t nonceSz,
                                     const void *aad, size_t aadSz,
                                     size_t tagSz, size_t sz);

/**
 * @brief Process payload data through an active AES-CCM session.
 *
 * @param dev AES-CCM device instance.
 * @param in  Input data.
 * @param out Output buffer.
 * @param sz  Data size in bytes.
 */
whal_Error whal_Stm32wb_AesCcm_Process(whal_AesCcm *dev,
                                       const void *in, void *out, size_t sz);

/**
 * @brief Finalize an AES-CCM session and produce the authentication tag.
 *
 * @param dev   AES-CCM device instance.
 * @param tag   Authentication tag output.
 * @param tagSz Tag size in bytes.
 */
whal_Error whal_Stm32wb_AesCcm_Finalize(whal_AesCcm *dev,
                                        void *tag, size_t tagSz);

/* ---- Singleton externs ---- */

/*
 * @brief Platform-owned AES + mode singletons. Defined in the driver TU
 * from the WHAL_CFG_STM32WB_AES*_DEV initializers in wolfHAL_board.h.
 */
extern const whal_Crypto  whal_Stm32wb_Aes_Dev;
extern const whal_AesEcb  whal_Stm32wb_AesEcb_Dev;
extern const whal_AesCbc  whal_Stm32wb_AesCbc_Dev;
extern const whal_AesCtr  whal_Stm32wb_AesCtr_Dev;
extern const whal_AesGcm  whal_Stm32wb_AesGcm_Dev;
extern const whal_AesGmac whal_Stm32wb_AesGmac_Dev;
extern const whal_AesCcm  whal_Stm32wb_AesCcm_Dev;

/* ---- Vtable externs ---- */

#ifndef WHAL_CFG_STM32WB_AES_INIT_DIRECT_API_MAPPING
extern const whal_CryptoDriver whal_Stm32wb_Aes_CryptoDriver;
#endif
#ifndef WHAL_CFG_STM32WB_AES_ECB_DIRECT_API_MAPPING
extern const whal_AesEcbDriver whal_Stm32wb_Aes_EcbDriver;
#endif
#ifndef WHAL_CFG_STM32WB_AES_CBC_DIRECT_API_MAPPING
extern const whal_AesCbcDriver whal_Stm32wb_Aes_CbcDriver;
#endif
#ifndef WHAL_CFG_STM32WB_AES_CTR_DIRECT_API_MAPPING
extern const whal_AesCtrDriver whal_Stm32wb_Aes_CtrDriver;
#endif
#ifndef WHAL_CFG_STM32WB_AES_GCM_DIRECT_API_MAPPING
extern const whal_AesGcmDriver whal_Stm32wb_Aes_GcmDriver;
#endif
#ifndef WHAL_CFG_STM32WB_AES_GMAC_DIRECT_API_MAPPING
extern const whal_AesGmacDriver whal_Stm32wb_Aes_GmacDriver;
#endif
#ifndef WHAL_CFG_STM32WB_AES_CCM_DIRECT_API_MAPPING
extern const whal_AesCcmDriver whal_Stm32wb_Aes_CcmDriver;
#endif

#endif /* WHAL_STM32WB_AES_H */
