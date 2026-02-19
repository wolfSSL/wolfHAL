#ifndef WHAL_SUPPLY_H
#define WHAL_SUPPLY_H

#include <wolfHAL/driver.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <stddef.h>

/*
 * @file supply.h
 * @brief Generic supply abstraction with compile-time dispatch.
 */

typedef struct whal_Supply whal_Supply;

#ifdef WHAL_RUNTIME_POLYMORPHISM

typedef struct {
    whal_Error (*init)(whal_Supply *dev);
    whal_Error (*deinit)(whal_Supply *dev);
    whal_Error (*enable)(whal_Supply *dev, void *supply);
    whal_Error (*disable)(whal_Supply *dev, void *supply);
} whal_SupplyOps;

#define WHAL_SUPPLY_OPS(DRIVER) {               \
    .init    = WHAL_DRV_FN(DRIVER, init),       \
    .deinit  = WHAL_DRV_FN(DRIVER, deinit),     \
    .enable  = WHAL_DRV_FN(DRIVER, enable),     \
    .disable = WHAL_DRV_FN(DRIVER, disable),    \
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

struct whal_Supply {
    const whal_Regmap regmap;
    void *cfg;
#ifdef WHAL_RUNTIME_POLYMORPHISM
    const whal_SupplyOps *ops;
#endif
};

#define WHAL_SUPPLY_DEV_DECLARE(NAME, DRIVER)                                       \
    extern whal_Supply whal_dev_##NAME;                                             \
    static inline whal_Error whal_dev_##NAME##_init(void) {                         \
        return WHAL_DRV_FN(DRIVER, init)(&whal_dev_##NAME);                         \
    }                                                                               \
    static inline whal_Error whal_dev_##NAME##_deinit(void) {                       \
        return WHAL_DRV_FN(DRIVER, deinit)(&whal_dev_##NAME);                       \
    }                                                                               \
    static inline whal_Error whal_dev_##NAME##_enable(void *supply) {               \
        return WHAL_DRV_FN(DRIVER, enable)(&whal_dev_##NAME, supply);               \
    }                                                                               \
    static inline whal_Error whal_dev_##NAME##_disable(void *supply) {              \
        return WHAL_DRV_FN(DRIVER, disable)(&whal_dev_##NAME, supply);              \
    }

#ifdef WHAL_RUNTIME_POLYMORPHISM
#define WHAL_SUPPLY_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)                           \
    static const whal_SupplyOps whal_dev_##NAME##_ops = WHAL_SUPPLY_OPS(DRIVER);    \
    whal_Supply whal_dev_##NAME = {                                                 \
        .regmap = REGMAP,                                                           \
        .cfg = CFG,                                                                 \
        .ops = &whal_dev_##NAME##_ops,                                              \
    }
#else
#define WHAL_SUPPLY_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)   \
    whal_Supply whal_dev_##NAME = {                         \
        .regmap = REGMAP,                                   \
        .cfg = CFG,                                         \
    }
#endif

#define WHAL_SUPPLY_INIT(NAME)              whal_dev_##NAME##_init()
#define WHAL_SUPPLY_DEINIT(NAME)            whal_dev_##NAME##_deinit()
#define WHAL_SUPPLY_ENABLE(NAME, supply)    whal_dev_##NAME##_enable(supply)
#define WHAL_SUPPLY_DISABLE(NAME, supply)   whal_dev_##NAME##_disable(supply)

#ifdef WHAL_RUNTIME_POLYMORPHISM

static inline whal_Error whal_Supply_Init(whal_Supply *dev) {
    return dev->ops->init(dev);
}
static inline whal_Error whal_Supply_Deinit(whal_Supply *dev) {
    return dev->ops->deinit(dev);
}
static inline whal_Error whal_Supply_Enable(whal_Supply *dev, void *supply) {
    return dev->ops->enable(dev, supply);
}
static inline whal_Error whal_Supply_Disable(whal_Supply *dev, void *supply) {
    return dev->ops->disable(dev, supply);
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

#endif /* WHAL_SUPPLY_H */
