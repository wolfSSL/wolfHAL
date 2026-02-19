#ifndef WHAL_PIC32CZ_SUPC_H
#define WHAL_PIC32CZ_SUPC_H

#include <stdint.h>
#include <wolfHAL/supply/supply.h>

/*
 * @file pic32cz_supc.h
 * @brief PIC32CZ supply controller (SUPC) driver configuration.
 */

/*
 * @brief PIC32CZ SUPC configuration parameters.
 */
typedef struct whal_Pic32czSupc_Cfg {
} whal_Pic32czSupc_Cfg;

typedef struct whal_Pic32czSupc_Supply {
    size_t enableMask;
} whal_Pic32czSupc_Supply;

whal_Error WHAL_DRV_FN(Pic32czSupc, init)(whal_Supply *supplyCtrl);
whal_Error WHAL_DRV_FN(Pic32czSupc, deinit)(whal_Supply *supplyCtrl);
whal_Error WHAL_DRV_FN(Pic32czSupc, enable)(whal_Supply *supplyCtrl, void *supply);
whal_Error WHAL_DRV_FN(Pic32czSupc, disable)(whal_Supply *supplyCtrl, void *supply);

#endif /* WHAL_PIC32CZ_SUPC_H */
