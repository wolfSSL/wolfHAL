#include <wolfHAL/error.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/timer/timer.h>
#include <wolfHAL/timer/systick.h>

#define SYSTICK_CSR_REG 0x00
#define SYSTICK_CSR_ENABLE WHAL_MASK(0)
#define SYSTICK_CSR_TICKINT WHAL_MASK(1)
#define SYSTICK_CSR_CLKSOURCE WHAL_MASK(2)
#define SYSTICK_CSR_COUNTFLAG WHAL_MASK(16)

#define SYSTICK_RVR_REG 0x04
#define SYSTICK_RVR_RELOAD WHAL_MASK_RANGE(23, 0)

whal_Error WHAL_DRV_FN(SysTick, init)(whal_Timer *timerDev)
{
    whal_SysTick_Cfg *cfg;
    const whal_Regmap *reg = &timerDev->regmap;
    
    if (!timerDev || !timerDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_SysTick_Cfg *)timerDev->cfg;

    whal_Reg_Update(reg->base, SYSTICK_CSR_REG,
                          SYSTICK_CSR_CLKSOURCE | SYSTICK_CSR_TICKINT,
                          whal_SetBits(SYSTICK_CSR_CLKSOURCE, cfg->clkSrc) |
                          whal_SetBits(SYSTICK_CSR_TICKINT, cfg->tickInt));

    whal_Reg_Update(reg->base, SYSTICK_RVR_REG, 
                    SYSTICK_RVR_RELOAD,
                    whal_SetBits(SYSTICK_RVR_RELOAD, cfg->cyclesPerTick));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(SysTick, deinit)(whal_Timer *timerDev)
{
    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(SysTick, start)(whal_Timer *timerDev)
{
    const whal_Regmap *reg = &timerDev->regmap;
    
    if (!timerDev || !timerDev->cfg) {
        return WHAL_EINVAL;
    }

    whal_Reg_Update(reg->base, SYSTICK_CSR_REG, SYSTICK_CSR_ENABLE,
                    whal_SetBits(SYSTICK_CSR_ENABLE, 1));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(SysTick, stop)(whal_Timer *timerDev)
{
    const whal_Regmap *reg = &timerDev->regmap;
    
    if (!timerDev || !timerDev->cfg) {
        return WHAL_EINVAL;
    }

    whal_Reg_Update(reg->base, SYSTICK_CSR_REG, SYSTICK_CSR_ENABLE,
                    whal_SetBits(SYSTICK_CSR_ENABLE, 0));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(SysTick, reset)(whal_Timer *timerDev)
{

    return WHAL_SUCCESS;
}

