/* crypto.h
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

#ifndef WHAL_CRYPTO_H
#define WHAL_CRYPTO_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>

/**
 * @file crypto.h
 * @brief Crypto accelerator device handle and per-algorithm API.
 *
 * whal_Crypto is a platform driver representing a crypto hardware peripheral.
 * It provides Init/Deinit for hardware lifecycle. Each algorithm gets its own
 * typed device struct (whal_AesGcm, whal_AesEcb, etc.) that references the
 * underlying whal_Crypto device and a per-algorithm driver vtable.
 */

/* ---- Common enums ---- */

/**
 * @brief Direction (encrypt or decrypt) for symmetric ciphers.
 */
typedef enum {
    WHAL_CRYPTO_ENCRYPT,
    WHAL_CRYPTO_DECRYPT,
} whal_Crypto_Dir;

/* ---- Hardware device (platform driver) ---- */

typedef struct whal_Crypto whal_Crypto;

/**
 * @brief Crypto hardware driver vtable (Init/Deinit only).
 */
typedef struct {
    whal_Error (*Init)(whal_Crypto *dev);
    whal_Error (*Deinit)(whal_Crypto *dev);
} whal_CryptoDriver;

/**
 * @brief Crypto hardware device instance.
 */
struct whal_Crypto {
    const size_t base;
    const whal_CryptoDriver *driver;
    const void *cfg;
};

/**
 * @brief Initialize a crypto hardware device.
 *
 * @param dev Crypto device instance.
 */
whal_Error whal_Crypto_Init(whal_Crypto *dev);

/**
 * @brief Deinitialize a crypto hardware device.
 *
 * @param dev Crypto device instance.
 */
whal_Error whal_Crypto_Deinit(whal_Crypto *dev);

/* ---- Per-algorithm device structs and vtables ---- */

/* AES-ECB */

typedef struct whal_AesEcb whal_AesEcb;

/**
 * @brief Driver vtable for AES-ECB.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_AesEcb *dev, whal_Crypto_Dir dir,
                          const void *key, size_t keySz,
                          const void *in, void *out, size_t sz);
    whal_Error (*Start)(whal_AesEcb *dev, whal_Crypto_Dir dir,
                        const void *key, size_t keySz);
    whal_Error (*Process)(whal_AesEcb *dev,
                          const void *in, void *out, size_t sz);
} whal_AesEcbDriver;

/**
 * @brief AES-ECB device instance.
 */
struct whal_AesEcb {
    whal_Crypto *crypto;
    const whal_AesEcbDriver *driver;
};

/**
 * @brief AES-ECB one-shot encrypt or decrypt.
 *
 * @param dev   AES-ECB device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 * @param in    Input data (multiple of 16 bytes).
 * @param out   Output buffer.
 * @param sz    Data size in bytes.
 */
whal_Error whal_AesEcb_Oneshot(whal_AesEcb *dev, whal_Crypto_Dir dir,
                               const void *key, size_t keySz,
                               const void *in, void *out, size_t sz);
/**
 * @brief Start an AES-ECB streaming session (load key).
 *
 * @param dev   AES-ECB device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 */
whal_Error whal_AesEcb_Start(whal_AesEcb *dev, whal_Crypto_Dir dir,
                             const void *key, size_t keySz);
/**
 * @brief Process data through an active AES-ECB session.
 *
 * @param dev AES-ECB device instance.
 * @param in  Input data (multiple of 16 bytes).
 * @param out Output buffer.
 * @param sz  Data size in bytes.
 */
whal_Error whal_AesEcb_Process(whal_AesEcb *dev,
                               const void *in, void *out, size_t sz);

/* AES-CBC */

typedef struct whal_AesCbc whal_AesCbc;

/**
 * @brief Driver vtable for AES-CBC.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_AesCbc *dev, whal_Crypto_Dir dir,
                          const void *key, size_t keySz,
                          const void *iv,
                          const void *in, void *out, size_t sz);
    whal_Error (*Start)(whal_AesCbc *dev, whal_Crypto_Dir dir,
                        const void *key, size_t keySz,
                        const void *iv);
    whal_Error (*Process)(whal_AesCbc *dev,
                          const void *in, void *out, size_t sz);
} whal_AesCbcDriver;

/**
 * @brief AES-CBC device instance.
 */
struct whal_AesCbc {
    whal_Crypto *crypto;
    const whal_AesCbcDriver *driver;
};

/**
 * @brief AES-CBC one-shot encrypt or decrypt.
 *
 * @param dev   AES-CBC device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 * @param iv    Initialization vector (16 bytes).
 * @param in    Input data (multiple of 16 bytes).
 * @param out   Output buffer.
 * @param sz    Data size in bytes.
 */
whal_Error whal_AesCbc_Oneshot(whal_AesCbc *dev, whal_Crypto_Dir dir,
                               const void *key, size_t keySz,
                               const void *iv,
                               const void *in, void *out, size_t sz);
/**
 * @brief Start an AES-CBC streaming session (load key and IV).
 *
 * @param dev   AES-CBC device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 * @param iv    Initialization vector (16 bytes).
 */
whal_Error whal_AesCbc_Start(whal_AesCbc *dev, whal_Crypto_Dir dir,
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
whal_Error whal_AesCbc_Process(whal_AesCbc *dev,
                               const void *in, void *out, size_t sz);

/* AES-CTR */

typedef struct whal_AesCtr whal_AesCtr;

/**
 * @brief Driver vtable for AES-CTR.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_AesCtr *dev, whal_Crypto_Dir dir,
                          const void *key, size_t keySz,
                          const void *iv,
                          const void *in, void *out, size_t sz);
    whal_Error (*Start)(whal_AesCtr *dev, whal_Crypto_Dir dir,
                        const void *key, size_t keySz,
                        const void *iv);
    whal_Error (*Process)(whal_AesCtr *dev,
                          const void *in, void *out, size_t sz);
} whal_AesCtrDriver;

/**
 * @brief AES-CTR device instance.
 */
struct whal_AesCtr {
    whal_Crypto *crypto;
    const whal_AesCtrDriver *driver;
};

/**
 * @brief AES-CTR one-shot encrypt or decrypt.
 *
 * @param dev   AES-CTR device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 * @param iv    Initial counter block (16 bytes).
 * @param in    Input data (multiple of 16 bytes).
 * @param out   Output buffer.
 * @param sz    Data size in bytes.
 */
whal_Error whal_AesCtr_Oneshot(whal_AesCtr *dev, whal_Crypto_Dir dir,
                               const void *key, size_t keySz,
                               const void *iv,
                               const void *in, void *out, size_t sz);
/**
 * @brief Start an AES-CTR streaming session (load key and counter).
 *
 * @param dev   AES-CTR device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 * @param iv    Initial counter block (16 bytes).
 */
whal_Error whal_AesCtr_Start(whal_AesCtr *dev, whal_Crypto_Dir dir,
                             const void *key, size_t keySz,
                             const void *iv);
/**
 * @brief Process data through an active AES-CTR session.
 *
 * @param dev AES-CTR device instance.
 * @param in  Input data (multiple of 16 bytes).
 * @param out Output buffer.
 * @param sz  Data size in bytes.
 */
whal_Error whal_AesCtr_Process(whal_AesCtr *dev,
                               const void *in, void *out, size_t sz);

/* AES-GCM */

typedef struct whal_AesGcm whal_AesGcm;

/**
 * @brief Driver vtable for AES-GCM.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_AesGcm *dev, whal_Crypto_Dir dir,
                          const void *key, size_t keySz,
                          const void *iv, size_t ivSz,
                          const void *aad, size_t aadSz,
                          const void *in, void *out, size_t sz,
                          void *tag, size_t tagSz);
    whal_Error (*Start)(whal_AesGcm *dev, whal_Crypto_Dir dir,
                        const void *key, size_t keySz,
                        const void *iv, size_t ivSz,
                        const void *aad, size_t aadSz);
    whal_Error (*Process)(whal_AesGcm *dev,
                          const void *in, void *out, size_t sz);
    whal_Error (*Finalize)(whal_AesGcm *dev,
                           void *tag, size_t tagSz);
} whal_AesGcmDriver;

/**
 * @brief AES-GCM device instance. The @c state pointer holds driver-specific
 *        streaming scratch (AAD/data lengths for GHASH final phase).
 */
struct whal_AesGcm {
    whal_Crypto *crypto;
    const whal_AesGcmDriver *driver;
    void *state;
};

/**
 * @brief AES-GCM one-shot authenticated encrypt or decrypt.
 *
 * @param dev   AES-GCM device instance.
 * @param dir   Encrypt or decrypt.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
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
whal_Error whal_AesGcm_Oneshot(whal_AesGcm *dev, whal_Crypto_Dir dir,
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
 * @param keySz Key size in bytes.
 * @param iv    Initialization vector.
 * @param ivSz  IV size in bytes (typically 12).
 * @param aad   Additional authenticated data.
 * @param aadSz AAD size in bytes.
 */
whal_Error whal_AesGcm_Start(whal_AesGcm *dev, whal_Crypto_Dir dir,
                             const void *key, size_t keySz,
                             const void *iv, size_t ivSz,
                             const void *aad, size_t aadSz);
/**
 * @brief Process payload data through an active AES-GCM session.
 *
 * @param dev AES-GCM device instance.
 * @param in  Input data.
 * @param out Output buffer.
 * @param sz  Data size in bytes. Every call except the last must be a
 *            multiple of 16 bytes.
 */
whal_Error whal_AesGcm_Process(whal_AesGcm *dev,
                               const void *in, void *out, size_t sz);
/**
 * @brief Finalize an AES-GCM session and produce the authentication tag.
 *
 * @param dev   AES-GCM device instance.
 * @param tag   Authentication tag output.
 * @param tagSz Tag size in bytes (up to 16).
 */
whal_Error whal_AesGcm_Finalize(whal_AesGcm *dev,
                                void *tag, size_t tagSz);

/* AES-GMAC */

typedef struct whal_AesGmac whal_AesGmac;

/**
 * @brief Driver vtable for AES-GMAC (authentication only, no payload).
 */
typedef struct {
    whal_Error (*Oneshot)(whal_AesGmac *dev,
                          const void *key, size_t keySz,
                          const void *iv, size_t ivSz,
                          const void *aad, size_t aadSz,
                          void *tag, size_t tagSz);
} whal_AesGmacDriver;

/**
 * @brief AES-GMAC device instance.
 */
struct whal_AesGmac {
    whal_Crypto *crypto;
    const whal_AesGmacDriver *driver;
};

/**
 * @brief AES-GMAC one-shot authentication (no payload).
 *
 * @param dev   AES-GMAC device instance.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 * @param iv    Initialization vector.
 * @param ivSz  IV size in bytes (typically 12).
 * @param aad   Authenticated data.
 * @param aadSz Data size in bytes.
 * @param tag   Authentication tag output.
 * @param tagSz Tag size in bytes (up to 16).
 */
whal_Error whal_AesGmac_Oneshot(whal_AesGmac *dev,
                                const void *key, size_t keySz,
                                const void *iv, size_t ivSz,
                                const void *aad, size_t aadSz,
                                void *tag, size_t tagSz);

/* AES-CCM */

typedef struct whal_AesCcm whal_AesCcm;

/**
 * @brief Driver vtable for AES-CCM.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_AesCcm *dev, whal_Crypto_Dir dir,
                          const void *key, size_t keySz,
                          const void *nonce, size_t nonceSz,
                          const void *aad, size_t aadSz,
                          const void *in, void *out, size_t sz,
                          void *tag, size_t tagSz);
    whal_Error (*Start)(whal_AesCcm *dev, whal_Crypto_Dir dir,
                        const void *key, size_t keySz,
                        const void *nonce, size_t nonceSz,
                        const void *aad, size_t aadSz,
                        size_t tagSz, size_t sz);
    whal_Error (*Process)(whal_AesCcm *dev,
                          const void *in, void *out, size_t sz);
    whal_Error (*Finalize)(whal_AesCcm *dev,
                           void *tag, size_t tagSz);
} whal_AesCcmDriver;

/**
 * @brief AES-CCM device instance. The @c state pointer holds driver-specific
 *        streaming scratch (AAD/data lengths for final-phase tag).
 */
struct whal_AesCcm {
    whal_Crypto *crypto;
    const whal_AesCcmDriver *driver;
    void *state;
};

/**
 * @brief AES-CCM one-shot authenticated encrypt or decrypt.
 *
 * @param dev     AES-CCM device instance.
 * @param dir     Encrypt or decrypt.
 * @param key     Key buffer.
 * @param keySz   Key size in bytes.
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
whal_Error whal_AesCcm_Oneshot(whal_AesCcm *dev, whal_Crypto_Dir dir,
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
 * @param keySz   Key size in bytes.
 * @param nonce   Nonce buffer.
 * @param nonceSz Nonce size in bytes (7-13).
 * @param aad     Additional authenticated data.
 * @param aadSz   AAD size in bytes.
 * @param tagSz   Tag size (encoded in B0); Finalize must pass the same value.
 * @param sz      Total payload size (encoded in B0); the Process calls must
 *                add up to exactly this many bytes.
 */
whal_Error whal_AesCcm_Start(whal_AesCcm *dev, whal_Crypto_Dir dir,
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
 * @param sz  Data size in bytes. Every call except the last must be a
 *            multiple of 16 bytes.
 */
whal_Error whal_AesCcm_Process(whal_AesCcm *dev,
                               const void *in, void *out, size_t sz);
/**
 * @brief Finalize an AES-CCM session and produce the authentication tag.
 *
 * @param dev   AES-CCM device instance.
 * @param tag   Authentication tag output.
 * @param tagSz Tag size in bytes; must match the tagSz passed to Start.
 */
whal_Error whal_AesCcm_Finalize(whal_AesCcm *dev,
                                void *tag, size_t tagSz);

/* SHA-1 */

typedef struct whal_Sha1 whal_Sha1;

/**
 * @brief Driver vtable for SHA-1.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_Sha1 *dev,
                          const void *in, size_t inSz,
                          void *digest, size_t digestSz);
    whal_Error (*Start)(whal_Sha1 *dev);
    whal_Error (*Process)(whal_Sha1 *dev, const void *in, size_t inSz);
    whal_Error (*Finalize)(whal_Sha1 *dev, void *digest, size_t digestSz);
} whal_Sha1Driver;

/**
 * @brief SHA-1 device instance.
 */
struct whal_Sha1 {
    whal_Crypto *crypto;
    const whal_Sha1Driver *driver;
};

/**
 * @brief SHA-1 one-shot hash.
 *
 * @param dev      SHA-1 device instance.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output buffer.
 * @param digestSz Output buffer size (must be 20).
 */
whal_Error whal_Sha1_Oneshot(whal_Sha1 *dev,
                             const void *in, size_t inSz,
                             void *digest, size_t digestSz);
/**
 * @brief Start a SHA-1 streaming session.
 *
 * @param dev SHA-1 device instance.
 */
whal_Error whal_Sha1_Start(whal_Sha1 *dev);
/**
 * @brief Process a chunk of data into an active SHA-1 session.
 *
 * @param dev  SHA-1 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_Sha1_Process(whal_Sha1 *dev, const void *in, size_t inSz);
/**
 * @brief Finalize a SHA-1 session and emit the digest.
 *
 * @param dev      SHA-1 device instance.
 * @param digest   Output buffer.
 * @param digestSz Output buffer size (must be 20).
 */
whal_Error whal_Sha1_Finalize(whal_Sha1 *dev, void *digest, size_t digestSz);

/* SHA-224 */

typedef struct whal_Sha224 whal_Sha224;

/**
 * @brief Driver vtable for SHA-224.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_Sha224 *dev,
                          const void *in, size_t inSz,
                          void *digest, size_t digestSz);
    whal_Error (*Start)(whal_Sha224 *dev);
    whal_Error (*Process)(whal_Sha224 *dev, const void *in, size_t inSz);
    whal_Error (*Finalize)(whal_Sha224 *dev, void *digest, size_t digestSz);
} whal_Sha224Driver;

/**
 * @brief SHA-224 device instance.
 */
struct whal_Sha224 {
    whal_Crypto *crypto;
    const whal_Sha224Driver *driver;
};

/**
 * @brief SHA-224 one-shot hash.
 *
 * @param dev      SHA-224 device instance.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output buffer.
 * @param digestSz Output buffer size (must be 28).
 */
whal_Error whal_Sha224_Oneshot(whal_Sha224 *dev,
                               const void *in, size_t inSz,
                               void *digest, size_t digestSz);
/**
 * @brief Start a SHA-224 streaming session.
 *
 * @param dev SHA-224 device instance.
 */
whal_Error whal_Sha224_Start(whal_Sha224 *dev);
/**
 * @brief Process a chunk of data into an active SHA-224 session.
 *
 * @param dev  SHA-224 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_Sha224_Process(whal_Sha224 *dev, const void *in, size_t inSz);
/**
 * @brief Finalize a SHA-224 session and emit the digest.
 *
 * @param dev      SHA-224 device instance.
 * @param digest   Output buffer.
 * @param digestSz Output buffer size (must be 28).
 */
whal_Error whal_Sha224_Finalize(whal_Sha224 *dev,
                                void *digest, size_t digestSz);

/* SHA-256 */

typedef struct whal_Sha256 whal_Sha256;

/**
 * @brief Driver vtable for SHA-256.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_Sha256 *dev,
                          const void *in, size_t inSz,
                          void *digest, size_t digestSz);
    whal_Error (*Start)(whal_Sha256 *dev);
    whal_Error (*Process)(whal_Sha256 *dev, const void *in, size_t inSz);
    whal_Error (*Finalize)(whal_Sha256 *dev, void *digest, size_t digestSz);
} whal_Sha256Driver;

/**
 * @brief SHA-256 device instance.
 */
struct whal_Sha256 {
    whal_Crypto *crypto;
    const whal_Sha256Driver *driver;
};

/**
 * @brief SHA-256 one-shot hash.
 *
 * @param dev      SHA-256 device instance.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output buffer.
 * @param digestSz Output buffer size (must be 32).
 */
whal_Error whal_Sha256_Oneshot(whal_Sha256 *dev,
                               const void *in, size_t inSz,
                               void *digest, size_t digestSz);
/**
 * @brief Start a SHA-256 streaming session.
 *
 * @param dev SHA-256 device instance.
 */
whal_Error whal_Sha256_Start(whal_Sha256 *dev);
/**
 * @brief Process a chunk of data into an active SHA-256 session.
 *
 * @param dev  SHA-256 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_Sha256_Process(whal_Sha256 *dev, const void *in, size_t inSz);
/**
 * @brief Finalize a SHA-256 session and emit the digest.
 *
 * @param dev      SHA-256 device instance.
 * @param digest   Output buffer.
 * @param digestSz Output buffer size (must be 32).
 */
whal_Error whal_Sha256_Finalize(whal_Sha256 *dev,
                                void *digest, size_t digestSz);

/* HMAC-SHA-1 */

typedef struct whal_HmacSha1 whal_HmacSha1;

/**
 * @brief Driver vtable for HMAC-SHA-1.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_HmacSha1 *dev,
                          const void *key, size_t keySz,
                          const void *in, size_t inSz,
                          void *digest, size_t digestSz);
    whal_Error (*Start)(whal_HmacSha1 *dev,
                        const void *key, size_t keySz);
    whal_Error (*Process)(whal_HmacSha1 *dev,
                          const void *in, size_t inSz);
    whal_Error (*Finalize)(whal_HmacSha1 *dev,
                           void *digest, size_t digestSz);
} whal_HmacSha1Driver;

/**
 * @brief HMAC-SHA-1 device instance. The @c state pointer holds driver-specific
 *        streaming scratch.
 */
struct whal_HmacSha1 {
    whal_Crypto *crypto;
    const whal_HmacSha1Driver *driver;
    void *state;
};

/**
 * @brief HMAC-SHA-1 one-shot MAC.
 *
 * @param dev      HMAC-SHA-1 device instance.
 * @param key      Key buffer.
 * @param keySz    Key size in bytes.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output MAC buffer.
 * @param digestSz Output buffer size (must be 20).
 */
whal_Error whal_HmacSha1_Oneshot(whal_HmacSha1 *dev,
                                 const void *key, size_t keySz,
                                 const void *in, size_t inSz,
                                 void *digest, size_t digestSz);
/**
 * @brief Start an HMAC-SHA-1 streaming session (load key).
 *
 * The key buffer is read again by Finalize, so it must stay valid and
 * unchanged until Finalize returns.
 *
 * @param dev   HMAC-SHA-1 device instance.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 */
whal_Error whal_HmacSha1_Start(whal_HmacSha1 *dev,
                               const void *key, size_t keySz);
/**
 * @brief Process a chunk of data into an active HMAC-SHA-1 session.
 *
 * @param dev  HMAC-SHA-1 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_HmacSha1_Process(whal_HmacSha1 *dev,
                                 const void *in, size_t inSz);
/**
 * @brief Finalize an HMAC-SHA-1 session and emit the MAC.
 *
 * @param dev      HMAC-SHA-1 device instance.
 * @param digest   Output MAC buffer.
 * @param digestSz Output buffer size (must be 20).
 */
whal_Error whal_HmacSha1_Finalize(whal_HmacSha1 *dev,
                                  void *digest, size_t digestSz);

/* HMAC-SHA-224 */

typedef struct whal_HmacSha224 whal_HmacSha224;

/**
 * @brief Driver vtable for HMAC-SHA-224.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_HmacSha224 *dev,
                          const void *key, size_t keySz,
                          const void *in, size_t inSz,
                          void *digest, size_t digestSz);
    whal_Error (*Start)(whal_HmacSha224 *dev,
                        const void *key, size_t keySz);
    whal_Error (*Process)(whal_HmacSha224 *dev,
                          const void *in, size_t inSz);
    whal_Error (*Finalize)(whal_HmacSha224 *dev,
                           void *digest, size_t digestSz);
} whal_HmacSha224Driver;

/**
 * @brief HMAC-SHA-224 device instance. The @c state pointer holds
 *        driver-specific streaming scratch.
 */
struct whal_HmacSha224 {
    whal_Crypto *crypto;
    const whal_HmacSha224Driver *driver;
    void *state;
};

/**
 * @brief HMAC-SHA-224 one-shot MAC.
 *
 * @param dev      HMAC-SHA-224 device instance.
 * @param key      Key buffer.
 * @param keySz    Key size in bytes.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output MAC buffer.
 * @param digestSz Output buffer size (must be 28).
 */
whal_Error whal_HmacSha224_Oneshot(whal_HmacSha224 *dev,
                                   const void *key, size_t keySz,
                                   const void *in, size_t inSz,
                                   void *digest, size_t digestSz);
/**
 * @brief Start an HMAC-SHA-224 streaming session (load key).
 *
 * The key buffer is read again by Finalize, so it must stay valid and
 * unchanged until Finalize returns.
 *
 * @param dev   HMAC-SHA-224 device instance.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 */
whal_Error whal_HmacSha224_Start(whal_HmacSha224 *dev,
                                 const void *key, size_t keySz);
/**
 * @brief Process a chunk of data into an active HMAC-SHA-224 session.
 *
 * @param dev  HMAC-SHA-224 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_HmacSha224_Process(whal_HmacSha224 *dev,
                                   const void *in, size_t inSz);
/**
 * @brief Finalize an HMAC-SHA-224 session and emit the MAC.
 *
 * @param dev      HMAC-SHA-224 device instance.
 * @param digest   Output MAC buffer.
 * @param digestSz Output buffer size (must be 28).
 */
whal_Error whal_HmacSha224_Finalize(whal_HmacSha224 *dev,
                                    void *digest, size_t digestSz);

/* HMAC-SHA-256 */

typedef struct whal_HmacSha256 whal_HmacSha256;

/**
 * @brief Driver vtable for HMAC-SHA-256.
 */
typedef struct {
    whal_Error (*Oneshot)(whal_HmacSha256 *dev,
                          const void *key, size_t keySz,
                          const void *in, size_t inSz,
                          void *digest, size_t digestSz);
    whal_Error (*Start)(whal_HmacSha256 *dev,
                        const void *key, size_t keySz);
    whal_Error (*Process)(whal_HmacSha256 *dev,
                          const void *in, size_t inSz);
    whal_Error (*Finalize)(whal_HmacSha256 *dev,
                           void *digest, size_t digestSz);
} whal_HmacSha256Driver;

/**
 * @brief HMAC-SHA-256 device instance. The @c state pointer holds
 *        driver-specific streaming scratch.
 */
struct whal_HmacSha256 {
    whal_Crypto *crypto;
    const whal_HmacSha256Driver *driver;
    void *state;
};

/**
 * @brief HMAC-SHA-256 one-shot MAC.
 *
 * @param dev      HMAC-SHA-256 device instance.
 * @param key      Key buffer.
 * @param keySz    Key size in bytes.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output MAC buffer.
 * @param digestSz Output buffer size (must be 32).
 */
whal_Error whal_HmacSha256_Oneshot(whal_HmacSha256 *dev,
                                   const void *key, size_t keySz,
                                   const void *in, size_t inSz,
                                   void *digest, size_t digestSz);
/**
 * @brief Start an HMAC-SHA-256 streaming session (load key).
 *
 * The key buffer is read again by Finalize, so it must stay valid and
 * unchanged until Finalize returns.
 *
 * @param dev   HMAC-SHA-256 device instance.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 */
whal_Error whal_HmacSha256_Start(whal_HmacSha256 *dev,
                                 const void *key, size_t keySz);
/**
 * @brief Process a chunk of data into an active HMAC-SHA-256 session.
 *
 * @param dev  HMAC-SHA-256 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_HmacSha256_Process(whal_HmacSha256 *dev,
                                   const void *in, size_t inSz);
/**
 * @brief Finalize an HMAC-SHA-256 session and emit the MAC.
 *
 * @param dev      HMAC-SHA-256 device instance.
 * @param digest   Output MAC buffer.
 * @param digestSz Output buffer size (must be 32).
 */
whal_Error whal_HmacSha256_Finalize(whal_HmacSha256 *dev,
                                    void *digest, size_t digestSz);

/* RSA / public-key math primitives live in <wolfHAL/crypto/pka.h> as
 * direct functions, not in this header. The crypto.h device-typed pattern
 * is for peripherals with per-instance configuration; the math primitives
 * don't need that layer. */

#endif /* WHAL_CRYPTO_H */
