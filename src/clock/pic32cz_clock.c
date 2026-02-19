#include <stdint.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/clock/pic32cz_clock.h>
#include <wolfHAL/supply/pic32cz_supc.h>
#include <wolfHAL/error.h>

/*
 * PIC32CZ Clock System Register Map
 *
 * The clock controller base address contains three subsystems at fixed offsets:
 * - OSCCTRL at 0x00000: Oscillator and PLL control
 * - GCLK at 0x10000: Generic clock controller
 * - MCLK at 0x12000: Main clock controller
 */

/* OSCCTRL - Oscillator Controller (base offset 0x00000) */
#define PIC32CZ_OSCCTRL 0x00000

/* PLL Control Register - enables PLL and selects reference/bandwidth */
#define PIC32CZ_OSCCTRL_PLLxCTRL_REG(pllInst) (0x40 + (pllInst * 20))
#define PIC32CZ_OSCCTRL_PLLxCTRL_ENABLE WHAL_MASK(1)         /* PLL enable */
#define PIC32CZ_OSCCTRL_PLLxCTRL_REFSEL WHAL_MASK_RANGE(10, 8) /* Reference select */
#define PIC32CZ_OSCCTRL_PLLxCTRL_BWSEL WHAL_MASK_RANGE(13, 11) /* Bandwidth select */

/* PLL Feedback Divider Register - sets VCO multiplication factor */
#define PIC32CZ_OSCCTRL_PLLxFBDIV_REG(pllInst) (0x44 + (pllInst * 20))
#define PIC32CZ_OSCCTRL_PLLxFBDIV WHAL_MASK_RANGE(9, 0) /* Feedback divider (16-1023) */

/* PLL Reference Divider Register - divides input reference clock */
#define PIC32CZ_OSCCTRL_PLLxREFDIV_REG(pllInst) (0x48 + (pllInst * 20))
#define PIC32CZ_OSCCTRL_PLLxREFDIV WHAL_MASK_RANGE(5, 0) /* Reference divider (1-63) */

/* PLL Post-Divider A Register - divides VCO for each output */
#define PIC32CZ_OSCCTRL_PLLxPOSTDIVA_REG(pllInst) (0x4C + (pllInst * 20))
#define PIC32CZ_OSCCTRL_PLLxPOSTDIVA_POSTDIV0 WHAL_MASK_RANGE(5, 0) /* Output 0 divider */
#define PIC32CZ_OSCCTRL_PLLxPOSTDIVA_OUTEN0 WHAL_MASK(7)            /* Output 0 enable */

/* OSCCTRL Status Register - PLL lock and oscillator ready flags */
#define PIC32CZ_OSCCTRL_STATUS_REG (PIC32CZ_OSCCTRL + 0x10)
#define PIC32CZ_OSCCTRL_STATUS_PLLxLOCK(pllInst) WHAL_MASK((24 + (pllInst)))

/* GCLK - Generic Clock Controller (base offset 0x10000) */
#define PIC32CZ_GCLK 0x10000

/* Generator Control Register - configures clock source and divider per generator */
#define PIC32CZ_GCLK_GENCTRLx_REG(gclkInst) ((PIC32CZ_GCLK + 0x20 + (gclkInst * 0x4)))
#define PIC32CZ_GCLK_GENCTRLx_SRC WHAL_MASK_RANGE(4, 0)   /* Source selection */
#define PIC32CZ_GCLK_GENCTRLx_GENEN WHAL_MASK(8)          /* Generator enable */
#define PIC32CZ_GCLK_GENCTRLx_DIV WHAL_MASK_RANGE(31, 16) /* Division factor */

/* Peripheral Channel Control Register - connects generator to peripheral */
#define PIC32CZ_GCLK_PCHCTRLx_REG(periphChannel) (PIC32CZ_GCLK + 0x80 + (periphChannel * 0x4))
#define PIC32CZ_GCLK_PCHCTRLx_GEN WHAL_MASK_RANGE(3, 0) /* Generator selection */
#define PIC32CZ_GCLK_PCHCTRLx_CHEN WHAL_MASK(6)         /* Channel enable */

/* GCLK Synchronization Busy Register - poll after writing GENCTRLx */
#define PIC32CZ_GCLK_SYNCBUSY_REG (PIC32CZ_GCLK + 0x04)
#define PIC32CZ_GCLK_SYNCBUSY_GENCTRLx(gclkInst) WHAL_MASK((2 + (gclkInst)))

/* MCLK - Main Clock Controller (base offset 0x12000) */
#define PIC32CZ_MCLK 0x12000

/* MCLK Interrupt Flag Register - clock ready status */
#define PIC32CZ_MCLK_INTFLAG_REG (PIC32CZ_MCLK + 0x08)
#define PIC32CZ_MCLK_INTFLAG_CKRDY WHAL_MASK(0) /* Clock ready */

/* CPU Clock Divider Register */
#define PIC32CZ_MCLK_DIV1_REG (PIC32CZ_MCLK + 0x10)
#define PIC32CZ_MCLK_DIV1 WHAL_MASK_RANGE(7, 0) /* CPU clock divider */

/* Peripheral Clock Mask Registers - enable/disable bus clocks to peripherals */
#define PIC32CZ_MCLK_CLKxMSK_REG(enableInst) (PIC32CZ_MCLK + 0x3C + (enableInst * 0x4))

/*
 * Example configuration for 150MHz CPU clock from DFLL48M:
 *
 * whal_Clock clk = {
 *     .regmap = { .base = 0x44000000 },  // Clock controller base
 *     .cfg = &(whal_Pic32czClock_Cfg) {
 *         .oscCtrlCfg = &(whal_Pic32czClockPll_OscCtrlCfg) {
 *             .pllInst = WHAL_PIC32CZ_PLL0,
 *             .refSel = WHAL_PIC32CZ_REFSEL_DFLL48M,
 *             .bwSel = WHAL_PIC32CZ_BWSEL_10MHz_TO_20MHz,
 *             .fbDiv = 225,   // VCO = (48MHz / 12) * 225 = 900MHz
 *             .refDiv = 12,
 *             .outCfgCount = 1,
 *             .outCfg = &(whal_Pic32czClockPll_OutCfg) {
 *                 .postDivMask = WHAL_PIC32CZ_POSTDIVMASK0,
 *                 .outEnMask = WHAL_PIC32CZ_OUTENMASK0,
 *                 .postDiv = 3,  // 900MHz / 3 = 300MHz
 *             },
 *         },
 *         .mclkCfg = &(whal_Pic32czClock_MclkCfg) {
 *             .div = 2,  // CPU clock = 300MHz / 2 = 150MHz
 *         },
 *         .gclkCfgCount = 1,
 *         .gclkCfg = &(whal_Pic32czClock_GclkCfg) {
 *             .gen = 0,
 *             .genSrc = WHAL_PIC32CZ_GENSRC_PLL0_CLOCKOUT0,
 *             .genDiv = 1,
 *         },
 *     },
 * };
 *
 * // Peripheral clock for SERCOM0
 * whal_Pic32czClock_Clk sercom0Clk = {
 *     .gclkPeriphChannel = 23,  // SERCOM0_CORE
 *     .gclkPeriphSrc = 0,       // Use generator 0
 *     .mclkEnableInst = 2,      // APBBMASK register
 *     .mclkEnableMask = WHAL_MASK(1),  // SERCOM0 bit
 * };
 */

whal_Error WHAL_DRV_FN(Pic32czClockPll, init)(whal_Clock *clkDev)
{
    whal_Pic32czClock_Cfg *cfg;
    whal_Pic32czClockPll_OscCtrlCfg *oscCtrlCfg;
    whal_Pic32czClock_MclkCfg *mclkCfg;

    size_t PLLxCTRL_REG;
    size_t PLLxFBDIV_REG;
    size_t PLLxREFDIV_REG;
    size_t PLLxPOSTDIVA_REG;
    size_t status;

    if (!clkDev) {
        return WHAL_EINVAL;
    }

    cfg = clkDev->cfg;
    oscCtrlCfg = cfg->oscCtrlCfg;
    mclkCfg = cfg->mclkCfg;

    /* Calculate register addresses for the selected PLL instance */
    PLLxCTRL_REG = PIC32CZ_OSCCTRL_PLLxCTRL_REG(oscCtrlCfg->pllInst);
    PLLxFBDIV_REG = PIC32CZ_OSCCTRL_PLLxFBDIV_REG(oscCtrlCfg->pllInst);
    PLLxREFDIV_REG = PIC32CZ_OSCCTRL_PLLxREFDIV_REG(oscCtrlCfg->pllInst);
    PLLxPOSTDIVA_REG = PIC32CZ_OSCCTRL_PLLxPOSTDIVA_REG(oscCtrlCfg->pllInst);

    /* Enable power supply for the PLL */
    WHAL_DRV_FN(Pic32czSupc, enable)(oscCtrlCfg->supplyCtrl, oscCtrlCfg->supply);

    /* Configure PLL feedback divider (sets VCO multiplication factor) */
    whal_Reg_Update(clkDev->regmap.base, PLLxFBDIV_REG, PIC32CZ_OSCCTRL_PLLxFBDIV,
                    whal_SetBits(PIC32CZ_OSCCTRL_PLLxFBDIV, oscCtrlCfg->fbDiv));

    /* Configure PLL reference divider (divides input clock before PLL) */
    whal_Reg_Update(clkDev->regmap.base, PLLxREFDIV_REG, PIC32CZ_OSCCTRL_PLLxREFDIV,
                    whal_SetBits(PIC32CZ_OSCCTRL_PLLxREFDIV, oscCtrlCfg->refDiv));

    /* Configure each PLL output with its post-divider and enable it */
    for (uint8_t i = 0; i < oscCtrlCfg->outCfgCount; ++i) {
        whal_Pic32czClockPll_OutCfg *outCfg = &oscCtrlCfg->outCfg[i];
        whal_Reg_Update(clkDev->regmap.base, PLLxPOSTDIVA_REG,
                        outCfg->outEnMask | outCfg->postDivMask,
                        whal_SetBits(outCfg->outEnMask, 1) |
                        whal_SetBits(outCfg->postDivMask, outCfg->postDiv));
    }

    /* Enable PLL with selected reference source and loop filter bandwidth */
    whal_Reg_Update(clkDev->regmap.base, PLLxCTRL_REG,
                    PIC32CZ_OSCCTRL_PLLxCTRL_ENABLE | PIC32CZ_OSCCTRL_PLLxCTRL_REFSEL | PIC32CZ_OSCCTRL_PLLxCTRL_BWSEL,
                    whal_SetBits(PIC32CZ_OSCCTRL_PLLxCTRL_ENABLE, 1) |
                    whal_SetBits(PIC32CZ_OSCCTRL_PLLxCTRL_REFSEL, oscCtrlCfg->refSel) |
                    whal_SetBits(PIC32CZ_OSCCTRL_PLLxCTRL_BWSEL, oscCtrlCfg->bwSel));

    /* Wait for PLL to lock */
    do {
        whal_Reg_Get(clkDev->regmap.base, PIC32CZ_OSCCTRL_STATUS_REG,
                     PIC32CZ_OSCCTRL_STATUS_PLLxLOCK(oscCtrlCfg->pllInst), &status);
    } while (!status);

    /* Configure CPU clock divider in MCLK */
    whal_Reg_Update(clkDev->regmap.base, PIC32CZ_MCLK_DIV1_REG, PIC32CZ_MCLK_DIV1,
                    whal_SetBits(PIC32CZ_MCLK_DIV1, mclkCfg->div));

    /* Wait for clock divider change to take effect */
    do {
        whal_Reg_Get(clkDev->regmap.base, PIC32CZ_MCLK_INTFLAG_REG,
                     PIC32CZ_MCLK_INTFLAG_CKRDY, &status);
    } while (!status);

    /* Configure each GCLK generator with its source and divider */
    for (uint8_t i = 0; i < cfg->gclkCfgCount; ++i) {
        whal_Pic32czClock_GclkCfg *gclkCfg = &cfg->gclkCfg[i];
        whal_Reg_Update(clkDev->regmap.base, PIC32CZ_GCLK_GENCTRLx_REG(gclkCfg->gen),
                        PIC32CZ_GCLK_GENCTRLx_SRC | PIC32CZ_GCLK_GENCTRLx_GENEN | PIC32CZ_GCLK_GENCTRLx_DIV,
                        whal_SetBits(PIC32CZ_GCLK_GENCTRLx_SRC, gclkCfg->genSrc) |
                        whal_SetBits(PIC32CZ_GCLK_GENCTRLx_GENEN, 1) |
                        whal_SetBits(PIC32CZ_GCLK_GENCTRLx_DIV, gclkCfg->genDiv));

        /* Wait for generator synchronization */
        do {
            whal_Reg_Get(clkDev->regmap.base, PIC32CZ_GCLK_SYNCBUSY_REG,
                         PIC32CZ_GCLK_SYNCBUSY_GENCTRLx(gclkCfg->gen), &status);
        } while (status);
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czClockPll, deinit)(whal_Clock *clkDev)
{
    if (!clkDev) {
        return WHAL_EINVAL;
    }

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czClockPll, enable)(whal_Clock *clkDev, const void *clk)
{
    const whal_Pic32czClock_Clk *pic32Clk = clk;

    if (!clkDev) {
        return WHAL_EINVAL;
    }

    /*
     * Enable peripheral clock in two steps:
     * 1. Connect the GCLK peripheral channel to a generator and enable it
     * 2. Enable the peripheral's bus clock in MCLK
     */

    /* Enable GCLK peripheral channel and connect to specified generator */
    whal_Reg_Update(clkDev->regmap.base, PIC32CZ_GCLK_PCHCTRLx_REG(pic32Clk->gclkPeriphChannel),
                    PIC32CZ_GCLK_PCHCTRLx_GEN | PIC32CZ_GCLK_PCHCTRLx_CHEN,
                    whal_SetBits(PIC32CZ_GCLK_PCHCTRLx_GEN, pic32Clk->gclkPeriphSrc) |
                    whal_SetBits(PIC32CZ_GCLK_PCHCTRLx_CHEN, 1));

    /* Enable bus clock for peripheral in MCLK mask register */
    whal_Reg_Update(clkDev->regmap.base, PIC32CZ_MCLK_CLKxMSK_REG(pic32Clk->mclkEnableInst),
                    pic32Clk->mclkEnableMask,
                    whal_SetBits(pic32Clk->mclkEnableMask, 1));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czClockPll, disable)(whal_Clock *clkDev, const void *clk)
{
    const whal_Pic32czClock_Clk *pic32Clk = clk;

    if (!clkDev) {
        return WHAL_EINVAL;
    }

    /* Disable bus clock for peripheral in MCLK mask register */
    whal_Reg_Update(clkDev->regmap.base, PIC32CZ_MCLK_CLKxMSK_REG(pic32Clk->mclkEnableInst),
                    pic32Clk->mclkEnableMask,
                    whal_SetBits(pic32Clk->mclkEnableMask, 0));

    /* Disable GCLK peripheral channel */
    whal_Reg_Update(clkDev->regmap.base, PIC32CZ_GCLK_PCHCTRLx_REG(pic32Clk->gclkPeriphChannel),
                    PIC32CZ_GCLK_PCHCTRLx_GEN | PIC32CZ_GCLK_PCHCTRLx_CHEN,
                    whal_SetBits(PIC32CZ_GCLK_PCHCTRLx_GEN, pic32Clk->gclkPeriphSrc) |
                    whal_SetBits(PIC32CZ_GCLK_PCHCTRLx_CHEN, 0));

    return WHAL_SUCCESS;
}

whal_Error WHAL_DRV_FN(Pic32czClockPll, getrate)(whal_Clock *clkDev, size_t *rateOut)
{
    if (!clkDev || !rateOut) {
        return WHAL_EINVAL;
    }

    /* TODO: Calculate actual clock rate from PLL and divider settings */
    *rateOut = 0;
    return WHAL_SUCCESS;
}

