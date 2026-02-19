#ifndef WHAL_RNG_H
#define WHAL_RNG_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/driver.h>
#include <wolfHAL/regmap.h>
#include <wolfHAL/error.h>

/*
 * @file rng.h
 * @brief Generic RNG abstraction with compile-time dispatch.
 */

typedef struct whal_Rng whal_Rng;

#ifdef WHAL_RUNTIME_POLYMORPHISM

typedef struct {
    whal_Error (*init)(whal_Rng *dev);
    whal_Error (*deinit)(whal_Rng *dev);
    whal_Error (*generate)(whal_Rng *dev, uint8_t *rngData, size_t rngDataSz);
} whal_RngOps;

#define WHAL_RNG_OPS(DRIVER) {                  \
    .init     = WHAL_DRV_FN(DRIVER, init),      \
    .deinit   = WHAL_DRV_FN(DRIVER, deinit),    \
    .generate = WHAL_DRV_FN(DRIVER, generate),  \
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

struct whal_Rng {
    const whal_Regmap regmap;
    void *cfg;
#ifdef WHAL_RUNTIME_POLYMORPHISM
    const whal_RngOps *ops;
#endif
};

#define WHAL_RNG_DEV_DECLARE(NAME, DRIVER)                                                          \
    extern whal_Rng whal_dev_##NAME;                                                                \
    static inline whal_Error whal_dev_##NAME##_init(void) {                                         \
        return WHAL_DRV_FN(DRIVER, init)(&whal_dev_##NAME);                                         \
    }                                                                                               \
    static inline whal_Error whal_dev_##NAME##_deinit(void) {                                       \
        return WHAL_DRV_FN(DRIVER, deinit)(&whal_dev_##NAME);                                       \
    }                                                                                               \
    static inline whal_Error whal_dev_##NAME##_generate(uint8_t *rngData, size_t rngDataSz) {       \
        return WHAL_DRV_FN(DRIVER, generate)(&whal_dev_##NAME, rngData, rngDataSz);                 \
    }

#ifdef WHAL_RUNTIME_POLYMORPHISM
#define WHAL_RNG_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)                          \
    static const whal_RngOps whal_dev_##NAME##_ops = WHAL_RNG_OPS(DRIVER);      \
    whal_Rng whal_dev_##NAME = {                                                \
        .regmap = REGMAP,                                                       \
        .cfg = CFG,                                                             \
        .ops = &whal_dev_##NAME##_ops,                                          \
    }
#else
#define WHAL_RNG_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)      \
    whal_Rng whal_dev_##NAME = {                            \
        .regmap = REGMAP,                                   \
        .cfg = CFG,                                         \
    }
#endif

#define WHAL_RNG_INIT(NAME)                         whal_dev_##NAME##_init()
#define WHAL_RNG_DEINIT(NAME)                       whal_dev_##NAME##_deinit()
#define WHAL_RNG_GENERATE(NAME, rngData, rngDataSz) whal_dev_##NAME##_generate(rngData, rngDataSz)

#ifdef WHAL_RUNTIME_POLYMORPHISM

static inline whal_Error whal_Rng_Init(whal_Rng *dev) {
    return dev->ops->init(dev);
}
static inline whal_Error whal_Rng_Deinit(whal_Rng *dev) {
    return dev->ops->deinit(dev);
}
static inline whal_Error whal_Rng_Generate(whal_Rng *dev, uint8_t *rngData,
                                           size_t rngDataSz) {
    return dev->ops->generate(dev, rngData, rngDataSz);
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

#endif /* WHAL_RNG_H */
