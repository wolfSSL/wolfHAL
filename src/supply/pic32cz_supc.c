#include <wolfHAL/bitops.h>
#include <wolfHAL/supply/pic32cz_supc.h>
#include <wolfHAL/error.h>

#define PIC32CZ_VREGCTRL_REG 0x1C
#define PIC32CZ_VREGCTRL_AVREGEN WHAL_MASK_RANGE(18, 16)


whal_Error WHAL_DRV_FN(Pic32czSupc, init)(whal_Supply *supplyCtrl)
{
    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czSupc, deinit)(whal_Supply *supplyCtrl)
{
    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czSupc, enable)(whal_Supply *supplyCtrl, void *supply)
{
    if (!supplyCtrl || !supply) {
        return WHAL_EINVAL;
    }

    whal_Pic32czSupc_Supply *picSupply = supply;

    whal_Reg_Update(supplyCtrl->regmap.base, PIC32CZ_VREGCTRL_REG, picSupply->enableMask,
                    whal_SetBits(picSupply->enableMask, 1));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czSupc, disable)(whal_Supply *supplyCtrl, void *supply)
{
    if (!supplyCtrl) {
        return WHAL_EINVAL;
    }

    whal_Pic32czSupc_Supply *picSupply = supply;

    whal_Reg_Update(supplyCtrl->regmap.base, PIC32CZ_VREGCTRL_REG, picSupply->enableMask,
                    whal_SetBits(picSupply->enableMask, 0));

    return WHAL_SUCCESS;
}

