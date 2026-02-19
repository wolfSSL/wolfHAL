#ifndef WHAL_SYSTICK_H
#define WHAL_SYSTICK_H

#include <stddef.h>
#include <wolfHAL/timer/timer.h>

/*
 * @file systick.h
 * @brief Configuration for the Cortex-M SysTick timer driver.
 */

/*
 * @brief Available SysTick clock sources.
 */
typedef enum {
    WHAL_SYSTICK_CLKSRC_EXT,
    WHAL_SYSTICK_CLKSRC_SYSCLK,
} whal_SysTick_ClkSrc;

/*
 * @brief Enable or disable the SysTick interrupt generation.
 */
typedef enum {
    WHAL_SYSTICK_TICKINT_DISABLED,
    WHAL_SYSTICK_TICKINT_ENABLED,
} whal_SysTick_TickInt;

/*
 * @brief SysTick configuration parameters.
 */
typedef struct {
    size_t cyclesPerTick;
    whal_SysTick_ClkSrc clkSrc;
    whal_SysTick_TickInt tickInt;
} whal_SysTick_Cfg;

whal_Error WHAL_DRV_FN(SysTick, init)(whal_Timer *timerDev);
whal_Error WHAL_DRV_FN(SysTick, deinit)(whal_Timer *timerDev);
whal_Error WHAL_DRV_FN(SysTick, start)(whal_Timer *timerDev);
whal_Error WHAL_DRV_FN(SysTick, stop)(whal_Timer *timerDev);
whal_Error WHAL_DRV_FN(SysTick, reset)(whal_Timer *timerDev);

#endif /* WHAL_SYSTICK_H */
