#ifndef WHAL_CLOCK_H
#define WHAL_CLOCK_H

#include <wolfHAL/driver.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <stddef.h>

/*
 * @file clock.h
 * @brief Generic clock abstraction with compile-time dispatch.
 */

typedef struct whal_Clock whal_Clock;

#ifdef WHAL_RUNTIME_POLYMORPHISM

typedef struct {
    whal_Error (*init)(whal_Clock *dev);
    whal_Error (*deinit)(whal_Clock *dev);
    whal_Error (*enable)(whal_Clock *dev, const void *clk);
    whal_Error (*disable)(whal_Clock *dev, const void *clk);
    whal_Error (*getrate)(whal_Clock *dev, size_t *rate);
} whal_ClockOps;

#define WHAL_CLOCK_OPS(DRIVER) {                \
    .init    = WHAL_DRV_FN(DRIVER, init),       \
    .deinit  = WHAL_DRV_FN(DRIVER, deinit),     \
    .enable  = WHAL_DRV_FN(DRIVER, enable),     \
    .disable = WHAL_DRV_FN(DRIVER, disable),    \
    .getrate = WHAL_DRV_FN(DRIVER, getrate),    \
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

struct whal_Clock {
    const whal_Regmap regmap;
    void *cfg;
#ifdef WHAL_RUNTIME_POLYMORPHISM
    const whal_ClockOps *ops;
#endif
};

#define WHAL_CLOCK_DEV_DECLARE(NAME, DRIVER)                                            \
    extern whal_Clock whal_dev_##NAME;                                                  \
    static inline whal_Error whal_dev_##NAME##_init(void) {                             \
        return WHAL_DRV_FN(DRIVER, init)(&whal_dev_##NAME);                             \
    }                                                                                   \
    static inline whal_Error whal_dev_##NAME##_deinit(void) {                           \
        return WHAL_DRV_FN(DRIVER, deinit)(&whal_dev_##NAME);                           \
    }                                                                                   \
    static inline whal_Error whal_dev_##NAME##_enable(const void *clk) {                \
        return WHAL_DRV_FN(DRIVER, enable)(&whal_dev_##NAME, clk);                      \
    }                                                                                   \
    static inline whal_Error whal_dev_##NAME##_disable(const void *clk) {               \
        return WHAL_DRV_FN(DRIVER, disable)(&whal_dev_##NAME, clk);                     \
    }                                                                                   \
    static inline whal_Error whal_dev_##NAME##_getrate(size_t *rate) {                  \
        return WHAL_DRV_FN(DRIVER, getrate)(&whal_dev_##NAME, rate);                    \
    }

#ifdef WHAL_RUNTIME_POLYMORPHISM
#define WHAL_CLOCK_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)                            \
    static const whal_ClockOps whal_dev_##NAME##_ops = WHAL_CLOCK_OPS(DRIVER);      \
    whal_Clock whal_dev_##NAME = {                                                  \
        .regmap = REGMAP,                                                           \
        .cfg = CFG,                                                                 \
        .ops = &whal_dev_##NAME##_ops,                                              \
    }
#else
#define WHAL_CLOCK_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)    \
    whal_Clock whal_dev_##NAME = {                          \
        .regmap = REGMAP,                                   \
        .cfg = CFG,                                         \
    }
#endif

#define WHAL_CLOCK_INIT(NAME)               whal_dev_##NAME##_init()
#define WHAL_CLOCK_DEINIT(NAME)             whal_dev_##NAME##_deinit()
#define WHAL_CLOCK_ENABLE(NAME, clk)        whal_dev_##NAME##_enable(clk)
#define WHAL_CLOCK_DISABLE(NAME, clk)       whal_dev_##NAME##_disable(clk)
#define WHAL_CLOCK_GETRATE(NAME, rate)      whal_dev_##NAME##_getrate(rate)

#ifdef WHAL_RUNTIME_POLYMORPHISM

static inline whal_Error whal_Clock_Init(whal_Clock *dev) {
    return dev->ops->init(dev);
}
static inline whal_Error whal_Clock_Deinit(whal_Clock *dev) {
    return dev->ops->deinit(dev);
}
static inline whal_Error whal_Clock_Enable(whal_Clock *dev, const void *clk) {
    return dev->ops->enable(dev, clk);
}
static inline whal_Error whal_Clock_Disable(whal_Clock *dev, const void *clk) {
    return dev->ops->disable(dev, clk);
}
static inline whal_Error whal_Clock_Getrate(whal_Clock *dev, size_t *rate) {
    return dev->ops->getrate(dev, rate);
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

#endif /* WHAL_CLOCK_H */
