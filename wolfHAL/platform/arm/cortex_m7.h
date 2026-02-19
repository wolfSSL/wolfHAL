#ifndef WHAL_CORTEX_M7_H
#define WHAL_CORTEX_M7_H

#include <wolfHAL/timer/systick.h>

#define WHAL_CORTEX_M7_SYSTICK_REGMAP   { .base = 0xE000E010, .size = 0x400 }
#define WHAL_CORTEX_M7_SYSTICK_DRIVER   SysTick

#endif /* WHAL_CORTEX_M7_H */
