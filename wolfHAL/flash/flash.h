#ifndef WHAL_FLASH_H
#define WHAL_FLASH_H

#include <wolfHAL/driver.h>
#include <wolfHAL/error.h>
#include <wolfHAL/regmap.h>
#include <stddef.h>
#include <stdint.h>

/*
 * @file flash.h
 * @brief Generic flash abstraction with compile-time dispatch.
 */

typedef struct whal_Flash whal_Flash;

#ifdef WHAL_RUNTIME_POLYMORPHISM

typedef struct {
    whal_Error (*init)(whal_Flash *dev);
    whal_Error (*deinit)(whal_Flash *dev);
    whal_Error (*lock)(whal_Flash *dev, size_t addr, size_t len);
    whal_Error (*unlock)(whal_Flash *dev, size_t addr, size_t len);
    whal_Error (*read)(whal_Flash *dev, size_t addr, uint8_t *data, size_t dataSz);
    whal_Error (*write)(whal_Flash *dev, size_t addr, const uint8_t *data, size_t dataSz);
    whal_Error (*erase)(whal_Flash *dev, size_t addr, size_t dataSz);
} whal_FlashOps;

#define WHAL_FLASH_OPS(DRIVER) {                \
    .init   = WHAL_DRV_FN(DRIVER, init),        \
    .deinit = WHAL_DRV_FN(DRIVER, deinit),      \
    .lock   = WHAL_DRV_FN(DRIVER, lock),        \
    .unlock = WHAL_DRV_FN(DRIVER, unlock),      \
    .read   = WHAL_DRV_FN(DRIVER, read),        \
    .write  = WHAL_DRV_FN(DRIVER, write),       \
    .erase  = WHAL_DRV_FN(DRIVER, erase),       \
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

struct whal_Flash {
    const whal_Regmap regmap;
    void *cfg;
#ifdef WHAL_RUNTIME_POLYMORPHISM
    const whal_FlashOps *ops;
#endif
};

#define WHAL_FLASH_DEV_DECLARE(NAME, DRIVER)                                                            \
    extern whal_Flash whal_dev_##NAME;                                                                  \
    static inline whal_Error whal_dev_##NAME##_init(void) {                                             \
        return WHAL_DRV_FN(DRIVER, init)(&whal_dev_##NAME);                                             \
    }                                                                                                   \
    static inline whal_Error whal_dev_##NAME##_deinit(void) {                                           \
        return WHAL_DRV_FN(DRIVER, deinit)(&whal_dev_##NAME);                                           \
    }                                                                                                   \
    static inline whal_Error whal_dev_##NAME##_lock(size_t addr, size_t len) {                          \
        return WHAL_DRV_FN(DRIVER, lock)(&whal_dev_##NAME, addr, len);                                  \
    }                                                                                                   \
    static inline whal_Error whal_dev_##NAME##_unlock(size_t addr, size_t len) {                        \
        return WHAL_DRV_FN(DRIVER, unlock)(&whal_dev_##NAME, addr, len);                                \
    }                                                                                                   \
    static inline whal_Error whal_dev_##NAME##_read(size_t addr, uint8_t *data, size_t dataSz) {        \
        return WHAL_DRV_FN(DRIVER, read)(&whal_dev_##NAME, addr, data, dataSz);                         \
    }                                                                                                   \
    static inline whal_Error whal_dev_##NAME##_write(size_t addr, const uint8_t *data, size_t dataSz) { \
        return WHAL_DRV_FN(DRIVER, write)(&whal_dev_##NAME, addr, data, dataSz);                        \
    }                                                                                                   \
    static inline whal_Error whal_dev_##NAME##_erase(size_t addr, size_t dataSz) {                      \
        return WHAL_DRV_FN(DRIVER, erase)(&whal_dev_##NAME, addr, dataSz);                              \
    }

#ifdef WHAL_RUNTIME_POLYMORPHISM
#define WHAL_FLASH_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)                            \
    static const whal_FlashOps whal_dev_##NAME##_ops = WHAL_FLASH_OPS(DRIVER);      \
    whal_Flash whal_dev_##NAME = {                                                  \
        .regmap = REGMAP,                                                           \
        .cfg = CFG,                                                                 \
        .ops = &whal_dev_##NAME##_ops,                                              \
    }
#else
#define WHAL_FLASH_DEV_DEFINE(NAME, DRIVER, REGMAP, CFG)    \
    whal_Flash whal_dev_##NAME = {                          \
        .regmap = REGMAP,                                   \
        .cfg = CFG,                                         \
    }
#endif

#define WHAL_FLASH_INIT(NAME)                           whal_dev_##NAME##_init()
#define WHAL_FLASH_DEINIT(NAME)                         whal_dev_##NAME##_deinit()
#define WHAL_FLASH_LOCK(NAME, addr, len)                whal_dev_##NAME##_lock(addr, len)
#define WHAL_FLASH_UNLOCK(NAME, addr, len)              whal_dev_##NAME##_unlock(addr, len)
#define WHAL_FLASH_READ(NAME, addr, data, dataSz)       whal_dev_##NAME##_read(addr, data, dataSz)
#define WHAL_FLASH_WRITE(NAME, addr, data, dataSz)      whal_dev_##NAME##_write(addr, data, dataSz)
#define WHAL_FLASH_ERASE(NAME, addr, dataSz)            whal_dev_##NAME##_erase(addr, dataSz)

#ifdef WHAL_RUNTIME_POLYMORPHISM

static inline whal_Error whal_Flash_Init(whal_Flash *dev) {
    return dev->ops->init(dev);
}
static inline whal_Error whal_Flash_Deinit(whal_Flash *dev) {
    return dev->ops->deinit(dev);
}
static inline whal_Error whal_Flash_Lock(whal_Flash *dev, size_t addr,
                                         size_t len) {
    return dev->ops->lock(dev, addr, len);
}
static inline whal_Error whal_Flash_Unlock(whal_Flash *dev, size_t addr,
                                           size_t len) {
    return dev->ops->unlock(dev, addr, len);
}
static inline whal_Error whal_Flash_Read(whal_Flash *dev, size_t addr,
                                         uint8_t *data, size_t dataSz) {
    return dev->ops->read(dev, addr, data, dataSz);
}
static inline whal_Error whal_Flash_Write(whal_Flash *dev, size_t addr,
                                          const uint8_t *data, size_t dataSz) {
    return dev->ops->write(dev, addr, data, dataSz);
}
static inline whal_Error whal_Flash_Erase(whal_Flash *dev, size_t addr,
                                          size_t dataSz) {
    return dev->ops->erase(dev, addr, dataSz);
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

#endif /* WHAL_FLASH_H */
