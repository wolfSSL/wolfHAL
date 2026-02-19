#ifndef WHAL_SPI_H
#define WHAL_SPI_H

#include <wolfHAL/driver.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <stdint.h>
#include <stddef.h>

/*
 * @file spi.h
 * @brief Generic SPI abstraction with compile-time dispatch.
 */

typedef struct whal_Spi whal_Spi;

#ifdef WHAL_RUNTIME_POLYMORPHISM

typedef struct {
    whal_Error (*init)(whal_Spi *dev);
    whal_Error (*deinit)(whal_Spi *dev);
    whal_Error (*sendrecv)(whal_Spi *dev, void *spiComCfg, const uint8_t *tx, size_t txLen, uint8_t *rx, size_t rxLen);
    whal_Error (*send)(whal_Spi *dev, void *spiComCfg, const uint8_t *data, size_t dataSz);
    whal_Error (*recv)(whal_Spi *dev, void *spiComCfg, uint8_t *data, size_t dataSz);
} whal_SpiOps;

#define WHAL_SPI_OPS(DRIVER) {                  \
    .init     = WHAL_DRV_FN(DRIVER, init),      \
    .deinit   = WHAL_DRV_FN(DRIVER, deinit),    \
    .sendrecv = WHAL_DRV_FN(DRIVER, sendrecv),  \
    .send     = WHAL_DRV_FN(DRIVER, send),      \
    .recv     = WHAL_DRV_FN(DRIVER, recv),       \
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

struct whal_Spi {
    const whal_Regmap regmap;
    void *cfg;
#ifdef WHAL_RUNTIME_POLYMORPHISM
    const whal_SpiOps *ops;
#endif
};

#define WHAL_SPI_DEV_DECLARE(NAME, DRIVER)                                                                                          \
    extern whal_Spi whal_dev_##NAME;                                                                                                \
    static inline whal_Error whal_dev_##NAME##_init(void) {                                                                         \
        return WHAL_DRV_FN(DRIVER, init)(&whal_dev_##NAME);                                                                         \
    }                                                                                                                               \
    static inline whal_Error whal_dev_##NAME##_deinit(void) {                                                                       \
        return WHAL_DRV_FN(DRIVER, deinit)(&whal_dev_##NAME);                                                                       \
    }                                                                                                                               \
    static inline whal_Error whal_dev_##NAME##_sendrecv(void *spiComCfg, const uint8_t *tx, size_t txLen, uint8_t *rx, size_t rxLen) {   \
        return WHAL_DRV_FN(DRIVER, sendrecv)(&whal_dev_##NAME, spiComCfg, tx, txLen, rx, rxLen);                                     \
    }                                                                                                                               \
    static inline whal_Error whal_dev_##NAME##_send(void *spiComCfg, const uint8_t *data, size_t dataSz) {                          \
        return WHAL_DRV_FN(DRIVER, send)(&whal_dev_##NAME, spiComCfg, data, dataSz);                                                \
    }                                                                                                                               \
    static inline whal_Error whal_dev_##NAME##_recv(void *spiComCfg, uint8_t *data, size_t dataSz) {                                \
        return WHAL_DRV_FN(DRIVER, recv)(&whal_dev_##NAME, spiComCfg, data, dataSz);                                                \
    }

#ifdef WHAL_RUNTIME_POLYMORPHISM
#define WHAL_SPI_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)                          \
    static const whal_SpiOps whal_dev_##NAME##_ops = WHAL_SPI_OPS(DRIVER);      \
    whal_Spi whal_dev_##NAME = {                                                \
        .regmap = REGMAP,                                                       \
        .cfg = CFG,                                                             \
        .ops = &whal_dev_##NAME##_ops,                                          \
    }
#else
#define WHAL_SPI_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)      \
    whal_Spi whal_dev_##NAME = {                            \
        .regmap = REGMAP,                                   \
        .cfg = CFG,                                         \
    }
#endif

#define WHAL_SPI_INIT(NAME)                                         whal_dev_##NAME##_init()
#define WHAL_SPI_DEINIT(NAME)                                       whal_dev_##NAME##_deinit()
#define WHAL_SPI_SENDRECV(NAME, spiComCfg, tx, txLen, rx, rxLen)    whal_dev_##NAME##_sendrecv(spiComCfg, tx, txLen, rx, rxLen)
#define WHAL_SPI_SEND(NAME, spiComCfg, data, dataSz)                whal_dev_##NAME##_send(spiComCfg, data, dataSz)
#define WHAL_SPI_RECV(NAME, spiComCfg, data, dataSz)                whal_dev_##NAME##_recv(spiComCfg, data, dataSz)

#ifdef WHAL_RUNTIME_POLYMORPHISM

static inline whal_Error whal_Spi_Init(whal_Spi *dev) {
    return dev->ops->init(dev);
}
static inline whal_Error whal_Spi_Deinit(whal_Spi *dev) {
    return dev->ops->deinit(dev);
}
static inline whal_Error whal_Spi_Sendrecv(whal_Spi *dev, void *spiComCfg,
                                           const uint8_t *tx, size_t txLen,
                                           uint8_t *rx, size_t rxLen) {
    return dev->ops->sendrecv(dev, spiComCfg, tx, txLen, rx, rxLen);
}
static inline whal_Error whal_Spi_Send(whal_Spi *dev, void *spiComCfg,
                                       const uint8_t *data, size_t dataSz) {
    return dev->ops->send(dev, spiComCfg, data, dataSz);
}
static inline whal_Error whal_Spi_Recv(whal_Spi *dev, void *spiComCfg,
                                       uint8_t *data, size_t dataSz) {
    return dev->ops->recv(dev, spiComCfg, data, dataSz);
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

#endif /* WHAL_SPI_H */
