/* stm32wba_hash.h
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

#ifndef WHAL_STM32WBA_HASH_H
#define WHAL_STM32WBA_HASH_H

#include <wolfHAL/crypto/crypto.h>
#include <wolfHAL/timeout.h>

/**
 * @file stm32wba_hash.h
 * @brief STM32WBA HASH hardware accelerator driver.
 *
 * The STM32WBA HASH peripheral supports SHA-1, SHA-224, SHA-256 and HMAC
 * variants. Each algorithm is exposed through a per-algorithm vtable with
 * Oneshot/Start/Process/Finalize operations.
 */

/**
 * @brief HASH device configuration.
 */
typedef struct {
    whal_Timeout *timeout;
} whal_Stm32wba_Hash_Cfg;

/* ---- Streaming state ---- */

/**
 * @brief HMAC streaming state (key pointer retained for outer-key phase).
 */
typedef struct {
    const void *key;
    size_t keySz;
} whal_Stm32wba_HmacSha1_State;

typedef whal_Stm32wba_HmacSha1_State whal_Stm32wba_HmacSha224_State;
typedef whal_Stm32wba_HmacSha1_State whal_Stm32wba_HmacSha256_State;

/* ---- Direct API mapping ---- */

#ifdef WHAL_CFG_STM32WBA_HASH_INIT_DIRECT_API_MAPPING
#define whal_Stm32wba_Hash_Init          whal_Crypto_Init
#define whal_Stm32wba_Hash_Deinit        whal_Crypto_Deinit
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_SHA1_DIRECT_API_MAPPING
#define whal_Stm32wba_Sha1_Oneshot       whal_Sha1_Oneshot
#define whal_Stm32wba_Sha1_Start         whal_Sha1_Start
#define whal_Stm32wba_Sha1_Process       whal_Sha1_Process
#define whal_Stm32wba_Sha1_Finalize      whal_Sha1_Finalize
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_SHA224_DIRECT_API_MAPPING
#define whal_Stm32wba_Sha224_Oneshot     whal_Sha224_Oneshot
#define whal_Stm32wba_Sha224_Start       whal_Sha224_Start
#define whal_Stm32wba_Sha224_Process     whal_Sha224_Process
#define whal_Stm32wba_Sha224_Finalize    whal_Sha224_Finalize
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_SHA256_DIRECT_API_MAPPING
#define whal_Stm32wba_Sha256_Oneshot     whal_Sha256_Oneshot
#define whal_Stm32wba_Sha256_Start       whal_Sha256_Start
#define whal_Stm32wba_Sha256_Process     whal_Sha256_Process
#define whal_Stm32wba_Sha256_Finalize    whal_Sha256_Finalize
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_HMAC_SHA1_DIRECT_API_MAPPING
#define whal_Stm32wba_HmacSha1_Oneshot   whal_HmacSha1_Oneshot
#define whal_Stm32wba_HmacSha1_Start     whal_HmacSha1_Start
#define whal_Stm32wba_HmacSha1_Process   whal_HmacSha1_Process
#define whal_Stm32wba_HmacSha1_Finalize  whal_HmacSha1_Finalize
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_HMAC_SHA224_DIRECT_API_MAPPING
#define whal_Stm32wba_HmacSha224_Oneshot  whal_HmacSha224_Oneshot
#define whal_Stm32wba_HmacSha224_Start    whal_HmacSha224_Start
#define whal_Stm32wba_HmacSha224_Process  whal_HmacSha224_Process
#define whal_Stm32wba_HmacSha224_Finalize whal_HmacSha224_Finalize
#endif

#ifdef WHAL_CFG_STM32WBA_HASH_HMAC_SHA256_DIRECT_API_MAPPING
#define whal_Stm32wba_HmacSha256_Oneshot  whal_HmacSha256_Oneshot
#define whal_Stm32wba_HmacSha256_Start    whal_HmacSha256_Start
#define whal_Stm32wba_HmacSha256_Process  whal_HmacSha256_Process
#define whal_Stm32wba_HmacSha256_Finalize whal_HmacSha256_Finalize
#endif

/* ---- Init / Deinit ---- */

/**
 * @brief Initialize the STM32WBA HASH peripheral.
 *
 * @param dev Crypto device instance.
 */
whal_Error whal_Stm32wba_Hash_Init(whal_Crypto *dev);

/**
 * @brief Deinitialize the STM32WBA HASH peripheral.
 *
 * @param dev Crypto device instance.
 */
whal_Error whal_Stm32wba_Hash_Deinit(whal_Crypto *dev);

/* ---- SHA-1 ---- */

/**
 * @brief SHA-1 one-shot hash.
 *
 * @param dev      SHA-1 device instance.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 20).
 */
whal_Error whal_Stm32wba_Sha1_Oneshot(whal_Sha1 *dev,
                                      const void *in, size_t inSz,
                                      void *digest, size_t digestSz);

/**
 * @brief Start a SHA-1 streaming session.
 *
 * @param dev SHA-1 device instance.
 */
whal_Error whal_Stm32wba_Sha1_Start(whal_Sha1 *dev);

/**
 * @brief Process data through an active SHA-1 session.
 *
 * @param dev  SHA-1 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_Stm32wba_Sha1_Process(whal_Sha1 *dev,
                                      const void *in, size_t inSz);

/**
 * @brief Finalize a SHA-1 session and produce the digest.
 *
 * @param dev      SHA-1 device instance.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 20).
 */
whal_Error whal_Stm32wba_Sha1_Finalize(whal_Sha1 *dev,
                                       void *digest, size_t digestSz);

/* ---- SHA-224 ---- */

/**
 * @brief SHA-224 one-shot hash.
 *
 * @param dev      SHA-224 device instance.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 28).
 */
whal_Error whal_Stm32wba_Sha224_Oneshot(whal_Sha224 *dev,
                                        const void *in, size_t inSz,
                                        void *digest, size_t digestSz);

/**
 * @brief Start a SHA-224 streaming session.
 *
 * @param dev SHA-224 device instance.
 */
whal_Error whal_Stm32wba_Sha224_Start(whal_Sha224 *dev);

/**
 * @brief Process data through an active SHA-224 session.
 *
 * @param dev  SHA-224 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_Stm32wba_Sha224_Process(whal_Sha224 *dev,
                                        const void *in, size_t inSz);

/**
 * @brief Finalize a SHA-224 session and produce the digest.
 *
 * @param dev      SHA-224 device instance.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 28).
 */
whal_Error whal_Stm32wba_Sha224_Finalize(whal_Sha224 *dev,
                                         void *digest, size_t digestSz);

/* ---- SHA-256 ---- */

/**
 * @brief SHA-256 one-shot hash.
 *
 * @param dev      SHA-256 device instance.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 32).
 */
whal_Error whal_Stm32wba_Sha256_Oneshot(whal_Sha256 *dev,
                                        const void *in, size_t inSz,
                                        void *digest, size_t digestSz);

/**
 * @brief Start a SHA-256 streaming session.
 *
 * @param dev SHA-256 device instance.
 */
whal_Error whal_Stm32wba_Sha256_Start(whal_Sha256 *dev);

/**
 * @brief Process data through an active SHA-256 session.
 *
 * @param dev  SHA-256 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_Stm32wba_Sha256_Process(whal_Sha256 *dev,
                                        const void *in, size_t inSz);

/**
 * @brief Finalize a SHA-256 session and produce the digest.
 *
 * @param dev      SHA-256 device instance.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 32).
 */
whal_Error whal_Stm32wba_Sha256_Finalize(whal_Sha256 *dev,
                                         void *digest, size_t digestSz);

/* ---- HMAC-SHA-1 ---- */

/**
 * @brief HMAC-SHA-1 one-shot.
 *
 * @param dev      HMAC-SHA-1 device instance.
 * @param key      Key buffer.
 * @param keySz    Key size in bytes.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 20).
 */
whal_Error whal_Stm32wba_HmacSha1_Oneshot(whal_HmacSha1 *dev,
                                          const void *key, size_t keySz,
                                          const void *in, size_t inSz,
                                          void *digest, size_t digestSz);

/**
 * @brief Start an HMAC-SHA-1 streaming session.
 *
 * @param dev   HMAC-SHA-1 device instance.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 */
whal_Error whal_Stm32wba_HmacSha1_Start(whal_HmacSha1 *dev,
                                         const void *key, size_t keySz);

/**
 * @brief Process data through an active HMAC-SHA-1 session.
 *
 * @param dev  HMAC-SHA-1 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_Stm32wba_HmacSha1_Process(whal_HmacSha1 *dev,
                                           const void *in, size_t inSz);

/**
 * @brief Finalize an HMAC-SHA-1 session and produce the digest.
 *
 * @param dev      HMAC-SHA-1 device instance.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 20).
 */
whal_Error whal_Stm32wba_HmacSha1_Finalize(whal_HmacSha1 *dev,
                                            void *digest, size_t digestSz);

/* ---- HMAC-SHA-224 ---- */

/**
 * @brief HMAC-SHA-224 one-shot.
 *
 * @param dev      HMAC-SHA-224 device instance.
 * @param key      Key buffer.
 * @param keySz    Key size in bytes.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 28).
 */
whal_Error whal_Stm32wba_HmacSha224_Oneshot(whal_HmacSha224 *dev,
                                            const void *key, size_t keySz,
                                            const void *in, size_t inSz,
                                            void *digest, size_t digestSz);

/**
 * @brief Start an HMAC-SHA-224 streaming session.
 *
 * @param dev   HMAC-SHA-224 device instance.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 */
whal_Error whal_Stm32wba_HmacSha224_Start(whal_HmacSha224 *dev,
                                           const void *key, size_t keySz);

/**
 * @brief Process data through an active HMAC-SHA-224 session.
 *
 * @param dev  HMAC-SHA-224 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_Stm32wba_HmacSha224_Process(whal_HmacSha224 *dev,
                                             const void *in, size_t inSz);

/**
 * @brief Finalize an HMAC-SHA-224 session and produce the digest.
 *
 * @param dev      HMAC-SHA-224 device instance.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 28).
 */
whal_Error whal_Stm32wba_HmacSha224_Finalize(whal_HmacSha224 *dev,
                                              void *digest, size_t digestSz);

/* ---- HMAC-SHA-256 ---- */

/**
 * @brief HMAC-SHA-256 one-shot.
 *
 * @param dev      HMAC-SHA-256 device instance.
 * @param key      Key buffer.
 * @param keySz    Key size in bytes.
 * @param in       Input data.
 * @param inSz     Input size in bytes.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 32).
 */
whal_Error whal_Stm32wba_HmacSha256_Oneshot(whal_HmacSha256 *dev,
                                            const void *key, size_t keySz,
                                            const void *in, size_t inSz,
                                            void *digest, size_t digestSz);

/**
 * @brief Start an HMAC-SHA-256 streaming session.
 *
 * @param dev   HMAC-SHA-256 device instance.
 * @param key   Key buffer.
 * @param keySz Key size in bytes.
 */
whal_Error whal_Stm32wba_HmacSha256_Start(whal_HmacSha256 *dev,
                                           const void *key, size_t keySz);

/**
 * @brief Process data through an active HMAC-SHA-256 session.
 *
 * @param dev  HMAC-SHA-256 device instance.
 * @param in   Input data.
 * @param inSz Input size in bytes.
 */
whal_Error whal_Stm32wba_HmacSha256_Process(whal_HmacSha256 *dev,
                                             const void *in, size_t inSz);

/**
 * @brief Finalize an HMAC-SHA-256 session and produce the digest.
 *
 * @param dev      HMAC-SHA-256 device instance.
 * @param digest   Output digest buffer.
 * @param digestSz Digest size in bytes (must be 32).
 */
whal_Error whal_Stm32wba_HmacSha256_Finalize(whal_HmacSha256 *dev,
                                              void *digest, size_t digestSz);

/* ---- Singleton externs ---- */

/*
 * @brief Platform-owned HASH + algorithm singletons. Defined in the driver
 * TU from the WHAL_CFG_STM32WBA_HASH*_DEV initializers in wolfHAL_board.h.
 */
extern const whal_Crypto     whal_Stm32wba_Hash_Dev;
extern const whal_Sha1       whal_Stm32wba_Sha1_Dev;
extern const whal_Sha224     whal_Stm32wba_Sha224_Dev;
extern const whal_Sha256     whal_Stm32wba_Sha256_Dev;
extern const whal_HmacSha1   whal_Stm32wba_HmacSha1_Dev;
extern const whal_HmacSha224 whal_Stm32wba_HmacSha224_Dev;
extern const whal_HmacSha256 whal_Stm32wba_HmacSha256_Dev;

/* ---- Vtable externs ---- */

#ifndef WHAL_CFG_STM32WBA_HASH_INIT_DIRECT_API_MAPPING
extern const whal_CryptoDriver whal_Stm32wba_Hash_CryptoDriver;
#endif
#ifndef WHAL_CFG_STM32WBA_HASH_SHA1_DIRECT_API_MAPPING
extern const whal_Sha1Driver whal_Stm32wba_Hash_Sha1Driver;
#endif
#ifndef WHAL_CFG_STM32WBA_HASH_SHA224_DIRECT_API_MAPPING
extern const whal_Sha224Driver whal_Stm32wba_Hash_Sha224Driver;
#endif
#ifndef WHAL_CFG_STM32WBA_HASH_SHA256_DIRECT_API_MAPPING
extern const whal_Sha256Driver whal_Stm32wba_Hash_Sha256Driver;
#endif
#ifndef WHAL_CFG_STM32WBA_HASH_HMAC_SHA1_DIRECT_API_MAPPING
extern const whal_HmacSha1Driver whal_Stm32wba_Hash_HmacSha1Driver;
#endif
#ifndef WHAL_CFG_STM32WBA_HASH_HMAC_SHA224_DIRECT_API_MAPPING
extern const whal_HmacSha224Driver whal_Stm32wba_Hash_HmacSha224Driver;
#endif
#ifndef WHAL_CFG_STM32WBA_HASH_HMAC_SHA256_DIRECT_API_MAPPING
extern const whal_HmacSha256Driver whal_Stm32wba_Hash_HmacSha256Driver;
#endif

#endif /* WHAL_STM32WBA_HASH_H */
