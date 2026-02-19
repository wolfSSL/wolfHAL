#ifndef WHAL_STM32WB_RNG_H
#define WHAL_STM32WB_RNG_H

#include <stdint.h>
#include <wolfHAL/rng/rng.h>
#include <wolfHAL/clock/clock.h>

/*
 * @file stm32wb_rng.h
 * @brief STM32WB RNG driver configuration.
 *
 * The STM32WB true random number generator provides 32-bit random values
 * from an analog noise source. The peripheral requires the RNG clock to
 * be enabled and produces one 32-bit word at a time via the DR register.
 */

/*
 * @brief RNG device configuration.
 */
typedef struct whal_Stm32wbRng_Cfg {
    whal_Clock *clkCtrl; /* Clock controller for RNG peripheral clock */
    const void *clk;     /* Clock descriptor */
} whal_Stm32wbRng_Cfg;

whal_Error WHAL_DRV_FN(Stm32wbRng, init)(whal_Rng *rngDev);
whal_Error WHAL_DRV_FN(Stm32wbRng, deinit)(whal_Rng *rngDev);
whal_Error WHAL_DRV_FN(Stm32wbRng, generate)(whal_Rng *rngDev, uint8_t *rngData, size_t rngDataSz);

#endif /* WHAL_STM32WB_RNG_H */
