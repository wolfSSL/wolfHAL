#ifndef WHAL_STM32WB_FLASH_H
#define WHAL_STM32WB_FLASH_H

#include <wolfHAL/flash/flash.h>
#include <wolfHAL/clock/clock.h>

/*
 * @file stm32wb_flash.h
 * @brief STM32WB flash driver configuration and helpers.
 *
 * The STM32WB embedded flash provides:
 * - Up to 1 MB of flash memory organized in 4 KB pages
 * - Double-word (64-bit) programming
 * - Page erase and mass erase operations
 * - Read-while-write capability on different banks
 * - Configurable wait states based on CPU frequency
 *
 * Flash must be unlocked before write/erase operations and locked
 * afterward for protection. Wait states must be configured appropriately
 * for the system clock frequency.
 */

/*
 * @brief Flash device configuration.
 */
typedef struct whal_Stm32wbFlash_Cfg {
    whal_Clock *clkCtrl;  /* Clock controller for flash interface clock */
    const void *clk;      /* Clock descriptor */
    size_t startAddr;     /* Flash base address (typically 0x08000000) */
    size_t size;          /* Flash size in bytes */
} whal_Stm32wbFlash_Cfg;

/*
 * @brief Flash access latency (wait states).
 *
 * The number of wait states must be configured based on the CPU frequency
 * and supply voltage. Insufficient wait states will cause flash read errors.
 *
 * Typical settings at VOS1 (1.2V):
 *   - 0 WS: up to 18 MHz
 *   - 1 WS: up to 36 MHz
 *   - 2 WS: up to 54 MHz
 *   - 3 WS: up to 64 MHz
 */
typedef enum whal_Stm32wbFlash_Latency {
    WHAL_STM32WB_FLASH_LATENCY_0, /* 0 wait states */
    WHAL_STM32WB_FLASH_LATENCY_1, /* 1 wait state */
    WHAL_STM32WB_FLASH_LATENCY_2, /* 2 wait states */
    WHAL_STM32WB_FLASH_LATENCY_3, /* 3 wait states */
} whal_Stm32wbFlash_Latency;

whal_Error WHAL_DRV_FN(Stm32wbFlash, init)(whal_Flash *flashDev);
whal_Error WHAL_DRV_FN(Stm32wbFlash, deinit)(whal_Flash *flashDev);
whal_Error WHAL_DRV_FN(Stm32wbFlash, lock)(whal_Flash *flashDev, size_t addr, size_t len);
whal_Error WHAL_DRV_FN(Stm32wbFlash, unlock)(whal_Flash *flashDev, size_t addr, size_t len);
whal_Error WHAL_DRV_FN(Stm32wbFlash, read)(whal_Flash *flashDev, size_t addr, uint8_t *data,
                             size_t dataSz);
whal_Error WHAL_DRV_FN(Stm32wbFlash, write)(whal_Flash *flashDev, size_t addr, const uint8_t *data,
                              size_t dataSz);
whal_Error WHAL_DRV_FN(Stm32wbFlash, erase)(whal_Flash *flashDev, size_t addr, size_t dataSz);

whal_Error whal_Stm32wbFlash_Ext_SetLatency(whal_Flash *flashDev, enum whal_Stm32wbFlash_Latency latency);

#endif /* WHAL_STM32WB_FLASH_H */
