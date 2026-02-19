#ifndef WHAL_PIC32CZ_FLASH_H
#define WHAL_PIC32CZ_FLASH_H

#include <wolfHAL/flash/flash.h>
#include <wolfHAL/clock/clock.h>

/*
 * @file pic32cz_flash.h
 * @brief PIC32CZ FCW (Flash Controller Write) driver configuration.
 *
 * The PIC32CZ flash is organized as:
 *   Page (erase unit)       = 4096 bytes
 *   Row (row-write unit)    = 1024 bytes
 *   Quad double word        = 32 bytes (8 x uint32)
 *   Single double word      = 8 bytes (2 x uint32)
 *
 * Write operations require double-word (8-byte) aligned addresses and sizes.
 * Erase operations erase full 4 KB pages.
 * Each write/erase operation requires an unlock key written to FCW_KEY.
 */

#define WHAL_PIC32CZ_FLASH_PAGE_SIZE    4096
#define WHAL_PIC32CZ_FLASH_DWORD_SIZE   8
#define WHAL_PIC32CZ_FLASH_QDWORD_SIZE  32

/*
 * @brief Flash device configuration.
 */
typedef struct whal_Pic32czFlash_Cfg {
    size_t startAddr;
    size_t size;
} whal_Pic32czFlash_Cfg;

whal_Error WHAL_DRV_FN(Pic32czFlash, init)(whal_Flash *flashDev);
whal_Error WHAL_DRV_FN(Pic32czFlash, deinit)(whal_Flash *flashDev);
whal_Error WHAL_DRV_FN(Pic32czFlash, lock)(whal_Flash *flashDev, size_t addr, size_t len);
whal_Error WHAL_DRV_FN(Pic32czFlash, unlock)(whal_Flash *flashDev, size_t addr, size_t len);
whal_Error WHAL_DRV_FN(Pic32czFlash, read)(whal_Flash *flashDev, size_t addr, uint8_t *data,
                             size_t dataSz);
whal_Error WHAL_DRV_FN(Pic32czFlash, write)(whal_Flash *flashDev, size_t addr, const uint8_t *data,
                              size_t dataSz);
whal_Error WHAL_DRV_FN(Pic32czFlash, erase)(whal_Flash *flashDev, size_t addr, size_t dataSz);

#endif /* WHAL_PIC32CZ_FLASH_H */
