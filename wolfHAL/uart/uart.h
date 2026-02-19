#ifndef WHAL_UART_H
#define WHAL_UART_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/driver.h>
#include <wolfHAL/regmap.h>
#include <wolfHAL/error.h>

/*
 * @file uart.h
 * @brief Generic UART abstraction with compile-time dispatch.
 */

typedef struct whal_Uart whal_Uart;

#ifdef WHAL_RUNTIME_POLYMORPHISM

typedef struct {
    whal_Error (*init)(whal_Uart *dev);
    whal_Error (*deinit)(whal_Uart *dev);
    whal_Error (*send)(whal_Uart *dev, const uint8_t *data, size_t dataSz);
    whal_Error (*recv)(whal_Uart *dev, uint8_t *data, size_t dataSz);
} whal_UartOps;

#define WHAL_UART_OPS(DRIVER) {                 \
    .init   = WHAL_DRV_FN(DRIVER, init),        \
    .deinit = WHAL_DRV_FN(DRIVER, deinit),      \
    .send   = WHAL_DRV_FN(DRIVER, send),        \
    .recv   = WHAL_DRV_FN(DRIVER, recv),         \
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

struct whal_Uart {
    const whal_Regmap regmap;
    void *cfg;
#ifdef WHAL_RUNTIME_POLYMORPHISM
    const whal_UartOps *ops;
#endif
};

#define WHAL_UART_DEV_DECLARE(NAME, DRIVER)                                             \
    extern whal_Uart whal_dev_##NAME;                                                   \
    static inline whal_Error whal_dev_##NAME##_init(void) {                             \
        return WHAL_DRV_FN(DRIVER, init)(&whal_dev_##NAME);                             \
    }                                                                                   \
    static inline whal_Error whal_dev_##NAME##_deinit(void) {                           \
        return WHAL_DRV_FN(DRIVER, deinit)(&whal_dev_##NAME);                           \
    }                                                                                   \
    static inline whal_Error whal_dev_##NAME##_send(const uint8_t *data, size_t dataSz) {   \
        return WHAL_DRV_FN(DRIVER, send)(&whal_dev_##NAME, data, dataSz);               \
    }                                                                                   \
    static inline whal_Error whal_dev_##NAME##_recv(uint8_t *data, size_t dataSz) {     \
        return WHAL_DRV_FN(DRIVER, recv)(&whal_dev_##NAME, data, dataSz);               \
    }

#ifdef WHAL_RUNTIME_POLYMORPHISM
#define WHAL_UART_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)                             \
    static const whal_UartOps whal_dev_##NAME##_ops = WHAL_UART_OPS(DRIVER);        \
    whal_Uart whal_dev_##NAME = {                                                   \
        .regmap = REGMAP,                                                           \
        .cfg = CFG,                                                                 \
        .ops = &whal_dev_##NAME##_ops,                                              \
    }
#else
#define WHAL_UART_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)     \
    whal_Uart whal_dev_##NAME = {                           \
        .regmap = REGMAP,                                   \
        .cfg = CFG,                                         \
    }
#endif

#define WHAL_UART_INIT(NAME)                    whal_dev_##NAME##_init()
#define WHAL_UART_DEINIT(NAME)                  whal_dev_##NAME##_deinit()
#define WHAL_UART_SEND(NAME, data, dataSz)      whal_dev_##NAME##_send(data, dataSz)
#define WHAL_UART_RECV(NAME, data, dataSz)      whal_dev_##NAME##_recv(data, dataSz)

#ifdef WHAL_RUNTIME_POLYMORPHISM

static inline whal_Error whal_Uart_Init(whal_Uart *dev) {
    return dev->ops->init(dev);
}
static inline whal_Error whal_Uart_Deinit(whal_Uart *dev) {
    return dev->ops->deinit(dev);
}
static inline whal_Error whal_Uart_Send(whal_Uart *dev, const uint8_t *data,
                                        size_t dataSz) {
    return dev->ops->send(dev, data, dataSz);
}
static inline whal_Error whal_Uart_Recv(whal_Uart *dev, uint8_t *data,
                                        size_t dataSz) {
    return dev->ops->recv(dev, data, dataSz);
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

#endif /* WHAL_UART_H */
