#ifndef WHAL_TIMER_H
#define WHAL_TIMER_H

#include <wolfHAL/driver.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <stddef.h>

/*
 * @file timer.h
 * @brief Generic timer abstraction with compile-time dispatch.
 */

typedef struct whal_Timer whal_Timer;

#ifdef WHAL_RUNTIME_POLYMORPHISM

typedef struct {
    whal_Error (*init)(whal_Timer *dev);
    whal_Error (*deinit)(whal_Timer *dev);
    whal_Error (*start)(whal_Timer *dev);
    whal_Error (*stop)(whal_Timer *dev);
    whal_Error (*reset)(whal_Timer *dev);
} whal_TimerOps;

#define WHAL_TIMER_OPS(DRIVER) {                \
    .init   = WHAL_DRV_FN(DRIVER, init),        \
    .deinit = WHAL_DRV_FN(DRIVER, deinit),      \
    .start  = WHAL_DRV_FN(DRIVER, start),       \
    .stop   = WHAL_DRV_FN(DRIVER, stop),        \
    .reset  = WHAL_DRV_FN(DRIVER, reset),       \
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

struct whal_Timer {
    const whal_Regmap regmap;
    void *cfg;
#ifdef WHAL_RUNTIME_POLYMORPHISM
    const whal_TimerOps *ops;
#endif
};

#define WHAL_TIMER_DEV_DECLARE(NAME, DRIVER)                                    \
    extern whal_Timer whal_dev_##NAME;                                          \
    static inline whal_Error whal_dev_##NAME##_init(void) {                     \
        return WHAL_DRV_FN(DRIVER, init)(&whal_dev_##NAME);                     \
    }                                                                           \
    static inline whal_Error whal_dev_##NAME##_deinit(void) {                   \
        return WHAL_DRV_FN(DRIVER, deinit)(&whal_dev_##NAME);                   \
    }                                                                           \
    static inline whal_Error whal_dev_##NAME##_start(void) {                    \
        return WHAL_DRV_FN(DRIVER, start)(&whal_dev_##NAME);                    \
    }                                                                           \
    static inline whal_Error whal_dev_##NAME##_stop(void) {                     \
        return WHAL_DRV_FN(DRIVER, stop)(&whal_dev_##NAME);                     \
    }                                                                           \
    static inline whal_Error whal_dev_##NAME##_reset(void) {                    \
        return WHAL_DRV_FN(DRIVER, reset)(&whal_dev_##NAME);                    \
    }

#ifdef WHAL_RUNTIME_POLYMORPHISM
#define WHAL_TIMER_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)                            \
    static const whal_TimerOps whal_dev_##NAME##_ops = WHAL_TIMER_OPS(DRIVER);      \
    whal_Timer whal_dev_##NAME = {                                                  \
        .regmap = REGMAP,                                                           \
        .cfg = CFG,                                                                 \
        .ops = &whal_dev_##NAME##_ops,                                              \
    }
#else
#define WHAL_TIMER_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)    \
    whal_Timer whal_dev_##NAME = {                          \
        .regmap = REGMAP,                                   \
        .cfg = CFG,                                         \
    }
#endif

#define WHAL_TIMER_INIT(NAME)       whal_dev_##NAME##_init()
#define WHAL_TIMER_DEINIT(NAME)     whal_dev_##NAME##_deinit()
#define WHAL_TIMER_START(NAME)      whal_dev_##NAME##_start()
#define WHAL_TIMER_STOP(NAME)       whal_dev_##NAME##_stop()
#define WHAL_TIMER_RESET(NAME)      whal_dev_##NAME##_reset()

#ifdef WHAL_RUNTIME_POLYMORPHISM

static inline whal_Error whal_Timer_Init(whal_Timer *dev) {
    return dev->ops->init(dev);
}
static inline whal_Error whal_Timer_Deinit(whal_Timer *dev) {
    return dev->ops->deinit(dev);
}
static inline whal_Error whal_Timer_Start(whal_Timer *dev) {
    return dev->ops->start(dev);
}
static inline whal_Error whal_Timer_Stop(whal_Timer *dev) {
    return dev->ops->stop(dev);
}
static inline whal_Error whal_Timer_Reset(whal_Timer *dev) {
    return dev->ops->reset(dev);
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

#endif /* WHAL_TIMER_H */
