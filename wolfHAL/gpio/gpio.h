#ifndef WHAL_GPIO_H
#define WHAL_GPIO_H

#include <wolfHAL/driver.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <stddef.h>

/*
 * @file gpio.h
 * @brief Generic GPIO abstraction with compile-time dispatch.
 */

typedef struct whal_Gpio whal_Gpio;

#ifdef WHAL_RUNTIME_POLYMORPHISM

typedef struct {
    whal_Error (*init)(whal_Gpio *dev);
    whal_Error (*deinit)(whal_Gpio *dev);
    whal_Error (*get)(whal_Gpio *dev, size_t pin, size_t *value);
    whal_Error (*set)(whal_Gpio *dev, size_t pin, size_t value);
} whal_GpioOps;

#define WHAL_GPIO_OPS(DRIVER) {                 \
    .init   = WHAL_DRV_FN(DRIVER, init),        \
    .deinit = WHAL_DRV_FN(DRIVER, deinit),      \
    .get    = WHAL_DRV_FN(DRIVER, get),          \
    .set    = WHAL_DRV_FN(DRIVER, set),          \
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

struct whal_Gpio {
    const whal_Regmap regmap;
    const void *cfg;
#ifdef WHAL_RUNTIME_POLYMORPHISM
    const whal_GpioOps *ops;
#endif
};

#define WHAL_GPIO_DEV_DECLARE(NAME, DRIVER)                                     \
    extern whal_Gpio whal_dev_##NAME;                                           \
    static inline whal_Error whal_dev_##NAME##_init(void) {                     \
        return WHAL_DRV_FN(DRIVER, init)(&whal_dev_##NAME);                     \
    }                                                                           \
    static inline whal_Error whal_dev_##NAME##_deinit(void) {                   \
        return WHAL_DRV_FN(DRIVER, deinit)(&whal_dev_##NAME);                   \
    }                                                                           \
    static inline whal_Error whal_dev_##NAME##_get(size_t pin, size_t *value) { \
        return WHAL_DRV_FN(DRIVER, get)(&whal_dev_##NAME, pin, value);          \
    }                                                                           \
    static inline whal_Error whal_dev_##NAME##_set(size_t pin, size_t value) {  \
        return WHAL_DRV_FN(DRIVER, set)(&whal_dev_##NAME, pin, value);          \
    }

#ifdef WHAL_RUNTIME_POLYMORPHISM
#define WHAL_GPIO_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)                         \
    static const whal_GpioOps whal_dev_##NAME##_ops = WHAL_GPIO_OPS(DRIVER);    \
    whal_Gpio whal_dev_##NAME = {                                               \
        .regmap = REGMAP,                                                       \
        .cfg = CFG,                                                             \
        .ops = &whal_dev_##NAME##_ops,                                          \
    }
#else
#define WHAL_GPIO_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)     \
    whal_Gpio whal_dev_##NAME = {                           \
        .regmap = REGMAP,                                   \
        .cfg = CFG,                                         \
    }
#endif

#define WHAL_GPIO_INIT(NAME)                whal_dev_##NAME##_init()
#define WHAL_GPIO_DEINIT(NAME)              whal_dev_##NAME##_deinit()
#define WHAL_GPIO_GET(NAME, pin, value)     whal_dev_##NAME##_get(pin, value)
#define WHAL_GPIO_SET(NAME, pin, value)     whal_dev_##NAME##_set(pin, value)

#ifdef WHAL_RUNTIME_POLYMORPHISM

static inline whal_Error whal_Gpio_Init(whal_Gpio *dev) {
    return dev->ops->init(dev);
}
static inline whal_Error whal_Gpio_Deinit(whal_Gpio *dev) {
    return dev->ops->deinit(dev);
}
static inline whal_Error whal_Gpio_Get(whal_Gpio *dev, size_t pin,
                                       size_t *value) {
    return dev->ops->get(dev, pin, value);
}
static inline whal_Error whal_Gpio_Set(whal_Gpio *dev, size_t pin,
                                       size_t value) {
    return dev->ops->set(dev, pin, value);
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

#endif /* WHAL_GPIO_H */
