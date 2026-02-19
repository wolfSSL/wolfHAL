#include <wolfHAL/error.h>
#include <wolfHAL/clock/clock.h>
#include <wolfHAL/clock/stm32wb_rcc.h>
#include <wolfHAL/flash/stm32wb_flash.h>
#include <wolfHAL/regmap.h>
#include <wolfHAL/bitops.h>

/*
 * STM32WB RCC Register Definitions
 *
 * The RCC (Reset and Clock Control) peripheral manages:
 * - Clock source selection and PLL configuration
 * - System clock (SYSCLK) routing
 * - Bus clock prescalers (AHB, APB1, APB2)
 * - Peripheral clock gating
 */

/* Clock Control Register - oscillator enables and status */
#define ST_RCC_CR_REG 0x000
#define ST_RCC_CR_MSIRANGE WHAL_MASK_RANGE(7, 4) /* MSI frequency range */
#define ST_RCC_CR_HSION_MASK WHAL_MASK(8)        /* HSI enable */
#define ST_RCC_CR_PLLON_MASK WHAL_MASK(24)       /* PLL enable */

/* Clock Configuration Register - clock source and prescaler selection */
#define ST_RCC_CFGR_REG 0x008
#define ST_RCC_CFGR_SW WHAL_MASK_RANGE(1, 0)       /* System clock switch */
#define ST_RCC_CFGR_SWS WHAL_MASK_RANGE(3, 2)      /* System clock switch status */
#define ST_RCC_CFGR_HPRE WHAL_MASK_RANGE(7, 4)     /* AHB prescaler */
#define ST_RCC_CFGR_PPRE1 WHAL_MASK_RANGE(10, 8)   /* APB1 prescaler */
#define ST_RCC_CFGR_PPRE2 WHAL_MASK_RANGE(13, 11)  /* APB2 prescaler */
#define ST_RCC_CFGR_STOPWUCK WHAL_MASK(15)         /* Wakeup clock after stop */
#define ST_RCC_CFGR_HPREF WHAL_MASK(16)            /* AHB prescaler flag */
#define ST_RCC_CFGR_PPRE1F WHAL_MASK(17)           /* APB1 prescaler flag */
#define ST_RCC_CFGR_PPRE2F WHAL_MASK(18)           /* APB2 prescaler flag */
#define ST_RCC_CFGR_MCOSEL WHAL_MASK_RANGE(27, 24) /* MCO source selection */
#define ST_RCC_CFGR_MCOPRE WHAL_MASK_RANGE(30, 28) /* MCO prescaler */

/* PLL Configuration Register */
#define ST_RCC_PLLCFGR_REG 0x00C
#define ST_RCC_PLLCFGR_PLLSRC_MASK WHAL_MASK_RANGE(1, 0)   /* PLL input source */
#define ST_RCC_PLLCFGR_PLLM_MASK   WHAL_MASK_RANGE(6, 4)   /* PLL input divider */
#define ST_RCC_PLLCFGR_PLLN_MASK   WHAL_MASK_RANGE(14, 8)  /* PLL VCO multiplier */
#define ST_RCC_PLLCFGR_PLLP_MASK   WHAL_MASK_RANGE(21, 17) /* PLLP output divider */
#define ST_RCC_PLLCFGR_PLLQ_MASK   WHAL_MASK_RANGE(27, 25) /* PLLQ output divider */
#define ST_RCC_PLLCFGR_PLLREN_MASK WHAL_MASK(28)           /* PLLR output enable */
#define ST_RCC_PLLCFGR_PLLR_MASK   WHAL_MASK_RANGE(31, 29) /* PLLR output divider */

#define ST_RCC_PLLCFGR_MASK \
        ST_RCC_PLLCFGR_PLLSRC_MASK | \
        ST_RCC_PLLCFGR_PLLM_MASK | \
        ST_RCC_PLLCFGR_PLLN_MASK | \
        ST_RCC_PLLCFGR_PLLP_MASK | \
        ST_RCC_PLLCFGR_PLLQ_MASK | \
        ST_RCC_PLLCFGR_PLLREN_MASK | \
        ST_RCC_PLLCFGR_PLLR_MASK

/* AHB2 Peripheral Clock Enable Register */
#define ST_RCC_AHB2ENR_REG 0x04C
#define ST_RCC_AHB2ENR_GPIOAEN WHAL_MASK(0)  /* GPIOA clock enable */
#define ST_RCC_AHB2ENR_GPIOBEN WHAL_MASK(1)  /* GPIOB clock enable */
#define ST_RCC_AHB2ENR_GPIOCEN WHAL_MASK(2)  /* GPIOC clock enable */
#define ST_RCC_AHB2ENR_GPIODEN WHAL_MASK(3)  /* GPIOD clock enable */
#define ST_RCC_AHB2ENR_GPIOEEN WHAL_MASK(4)  /* GPIOE clock enable */
#define ST_RCC_AHB2ENR_GPIOHEN WHAL_MASK(7)  /* GPIOH clock enable */
#define ST_RCC_AHB2ENR_ADCEN   WHAL_MASK(13) /* ADC clock enable */
#define ST_RCC_AHB2ENR_AES1EN  WHAL_MASK(16) /* AES1 clock enable */

/* AHB3 Peripheral Clock Enable Register */
#define ST_RCC_AHB3ENR_REG 0x50
#define ST_RCC_AHB3ENR_FLASHEN WHAL_MASK(25) /* Flash interface clock enable */

/* APB1 Peripheral Clock Enable Register 2 */
#define ST_RCC_APB1ENR2_REG 0x05C
#define ST_RCC_APB1ENR2_LPUART1EN WHAL_MASK(0) /* LPUART1 clock enable */
#define ST_RCC_APB1ENR2_LPTIM2EN  WHAL_MASK(5) /* LPTIM2 clock enable */

/* Clock Recovery RC Register - HSI48 oscillator control */
#define ST_RCC_CRRCR_REG 0x098
#define ST_RCC_CRRCR_HSI48ON_MASK  WHAL_MASK(0) /* HSI48 oscillator enable */
#define ST_RCC_CRRCR_HSI48RDY_MASK WHAL_MASK(1) /* HSI48 oscillator ready */

whal_Error WHAL_DRV_FN(Stm32wbRccPll, init)(whal_Clock *clkDev)
{
    whal_Error err;
    whal_Stm32wbRcc_Cfg *cfg;

    if (!clkDev || !clkDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wbRcc_Cfg *)clkDev->cfg;
    whal_Stm32wbRcc_PllClkCfg *pllCfg = cfg->sysClkCfg;

    /*
     * Flash latency must be set BEFORE increasing clock speed to ensure
     * the flash can keep up with the new frequency.
     */
    err = whal_Stm32wbFlash_Ext_SetLatency(cfg->flash, cfg->flashLatency);
    if (err) {
        return err;
    }

    /* Select system clock source (PLL in this case) */
    whal_Reg_Update(clkDev->regmap.base, ST_RCC_CFGR_REG, ST_RCC_CFGR_SW,
                    whal_SetBits(ST_RCC_CFGR_SW, cfg->sysClkSrc));

    /* Configure PLL: source, dividers, and multiplier */
    whal_Reg_Update(clkDev->regmap.base,
                    ST_RCC_PLLCFGR_REG, ST_RCC_PLLCFGR_MASK,
                    whal_SetBits(ST_RCC_PLLCFGR_PLLSRC_MASK, pllCfg->clkSrc) |
                    whal_SetBits(ST_RCC_PLLCFGR_PLLM_MASK, pllCfg->m) |
                    whal_SetBits(ST_RCC_PLLCFGR_PLLN_MASK, pllCfg->n) |
                    whal_SetBits(ST_RCC_PLLCFGR_PLLP_MASK, pllCfg->p) |
                    whal_SetBits(ST_RCC_PLLCFGR_PLLQ_MASK, pllCfg->q) |
                    whal_SetBits(ST_RCC_PLLCFGR_PLLREN_MASK, 1) |
                    whal_SetBits(ST_RCC_PLLCFGR_PLLR_MASK, pllCfg->r));

    /* Enable the PLL */
    whal_Reg_Update(clkDev->regmap.base, ST_RCC_CR_REG,
                    ST_RCC_CR_PLLON_MASK,
                    whal_SetBits(ST_RCC_CR_PLLON_MASK, 1));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbRccMsi, init)(whal_Clock *clkDev)
{
    whal_Error err;
    whal_Stm32wbRcc_Cfg *cfg;

    if (!clkDev || !clkDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wbRcc_Cfg *)clkDev->cfg;
    whal_Stm32wbRcc_MsiClkCfg *msiCfg = cfg->sysClkCfg;

    /* Set flash latency for target MSI frequency */
    err = whal_Stm32wbFlash_Ext_SetLatency(cfg->flash, cfg->flashLatency);
    if (err) {
        return err;
    }

    /* Select MSI as system clock source */
    whal_Reg_Update(clkDev->regmap.base, ST_RCC_CFGR_REG, ST_RCC_CFGR_SW,
                    whal_SetBits(ST_RCC_CFGR_SW, WHAL_STM32WB_RCC_SYSCLK_SRC_MSI));

    /* Set MSI frequency range */
    whal_Reg_Update(clkDev->regmap.base, ST_RCC_CR_REG, ST_RCC_CR_MSIRANGE,
                    whal_SetBits(ST_RCC_CR_MSIRANGE, msiCfg->freq));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbRccPll, deinit)(whal_Clock *clkDev)
{
    whal_Stm32wbRcc_Cfg *cfg;

    if (!clkDev || !clkDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wbRcc_Cfg *)clkDev->cfg;

    /* Switch back to MSI before disabling PLL */
    whal_Reg_Update(clkDev->regmap.base, ST_RCC_CFGR_REG, ST_RCC_CFGR_SW,
                    whal_SetBits(ST_RCC_CFGR_SW, WHAL_STM32WB_RCC_SYSCLK_SRC_MSI));

    /* Reset MSI to default 4 MHz */
    whal_Reg_Update(clkDev->regmap.base, ST_RCC_CR_REG, ST_RCC_CR_MSIRANGE,
                    whal_SetBits(ST_RCC_CR_MSIRANGE, WHAL_STM32WB_RCC_MSIRANGE_4MHz));

    /* Disable the PLL */
    whal_Reg_Update(clkDev->regmap.base, ST_RCC_CR_REG,
                    ST_RCC_CR_PLLON_MASK,
                    whal_SetBits(ST_RCC_CR_PLLON_MASK, 0));

    /* Reduce flash latency now that clock is slower */
    whal_Stm32wbFlash_Ext_SetLatency(cfg->flash, WHAL_STM32WB_FLASH_LATENCY_0);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbRccMsi, deinit)(whal_Clock *clkDev)
{
    whal_Stm32wbRcc_Cfg *cfg;

    if (!clkDev || !clkDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wbRcc_Cfg *)clkDev->cfg;

    whal_Reg_Update(clkDev->regmap.base, ST_RCC_CFGR_REG, ST_RCC_CFGR_SW,
                    whal_SetBits(ST_RCC_CFGR_SW, WHAL_STM32WB_RCC_SYSCLK_SRC_MSI));

    whal_Reg_Update(clkDev->regmap.base, ST_RCC_CR_REG, ST_RCC_CR_MSIRANGE,
                    whal_SetBits(ST_RCC_CR_MSIRANGE, WHAL_STM32WB_RCC_MSIRANGE_4MHz));

    whal_Stm32wbFlash_Ext_SetLatency(cfg->flash, WHAL_STM32WB_FLASH_LATENCY_0);

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbRccPll, enable)(whal_Clock *clkDev, const void *clk)
{
    whal_Stm32wbRcc_Clk *stClk = (whal_Stm32wbRcc_Clk *)clk;

    /* Set the peripheral's enable bit in the appropriate RCC enable register */
    whal_Reg_Update(clkDev->regmap.base, stClk->regOffset, stClk->enableMask,
                    whal_SetBits(stClk->enableMask, 1));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbRccPll, disable)(whal_Clock *clkDev, const void *clk)
{
    whal_Stm32wbRcc_Clk *stClk = (whal_Stm32wbRcc_Clk *)clk;

    /* Clear the peripheral's enable bit to gate its clock */
    whal_Reg_Update(clkDev->regmap.base, stClk->regOffset, stClk->enableMask,
                    whal_SetBits(stClk->enableMask, 0));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbRccPll, getrate)(whal_Clock *clkDev, size_t *rateOut)
{
    whal_Stm32wbRcc_Cfg *cfg;

    if (!clkDev || !clkDev->cfg) {
        return WHAL_EINVAL;
    }

    cfg = (whal_Stm32wbRcc_Cfg *)clkDev->cfg;
    whal_Stm32wbRcc_PllClkCfg *pllClkCfg = cfg->sysClkCfg;

    /*
     * Calculate PLL output frequency:
     *   f_pllr = ((f_src / (m + 1)) * n) / (r + 1)
     */
    size_t srcFreq;
    size_t pllm = pllClkCfg->m + 1;
    size_t plln = pllClkCfg->n;
    size_t pllr = pllClkCfg->r + 1;

    /* Determine source frequency based on PLL input selection */
    if (pllClkCfg->clkSrc == WHAL_STM32WB_RCC_PLLCLK_SRC_MSI) {
        srcFreq = 4000000;
    }
    else {
        return WHAL_EINVAL;
    }

    *rateOut = ((srcFreq / pllm) * plln) / pllr;
    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Stm32wbRccMsi, getrate)(whal_Clock *clkDev, size_t *rateOut)
{
    size_t msiRange;

    if (!clkDev || !rateOut) {
        return WHAL_EINVAL;
    }

    /* Read current MSI range from hardware */
    whal_Reg_Get(clkDev->regmap.base, ST_RCC_CR_REG, ST_RCC_CR_MSIRANGE, &msiRange);

    /* Map range setting to frequency in Hz */
    switch (msiRange) {
    case WHAL_STM32WB_RCC_MSIRANGE_100kHz:
        *rateOut = 100000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_200kHz:
        *rateOut = 200000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_400kHz:
        *rateOut = 400000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_800kHz:
        *rateOut = 800000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_1MHz:
        *rateOut = 1000000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_2MHz:
        *rateOut = 2000000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_4MHz:
        *rateOut = 4000000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_8MHz:
        *rateOut = 8000000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_16MHz:
        *rateOut = 16000000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_24MHz:
        *rateOut = 24000000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_32MHz:
        *rateOut = 32000000;
        break;
    case WHAL_STM32WB_RCC_MSIRANGE_48MHz:
        *rateOut = 48000000;
        break;
    }

    return WHAL_SUCCESS;
}

whal_Error whal_Stm32wbRcc_Ext_EnableHsi48(whal_Clock *clkDev, uint8_t enable)
{
    if (!clkDev) {
        return WHAL_EINVAL;
    }

    whal_Reg_Update(clkDev->regmap.base, ST_RCC_CRRCR_REG, ST_RCC_CRRCR_HSI48ON_MASK,
                    whal_SetBits(ST_RCC_CRRCR_HSI48ON_MASK, enable));

    if (enable) {
        size_t rdy;
        do {
            whal_Reg_Get(clkDev->regmap.base, ST_RCC_CRRCR_REG,
                         ST_RCC_CRRCR_HSI48RDY_MASK, &rdy);
        } while (!rdy);
    }

    return WHAL_SUCCESS;
}

