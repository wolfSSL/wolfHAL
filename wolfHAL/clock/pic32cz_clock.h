#ifndef WHAL_PIC32CZ_CLOCK_H
#define WHAL_PIC32CZ_CLOCK_H

#include <stdint.h>
#include <wolfHAL/bitops.h>
#include <wolfHAL/clock/clock.h>
#include <wolfHAL/supply/supply.h>

/*
 * @file pic32cz_clock.h
 * @brief PIC32CZ clock system driver configuration.
 *
 * The PIC32CZ clock system consists of three main subsystems:
 *
 * 1. OSCCTRL (Oscillator Controller) - Contains PLLs and oscillator sources
 *    - PLL0/PLL1: Fractional PLLs with up to 4 independent outputs each
 *    - Reference sources: GCLK, XOSC, or internal DFLL48M
 *
 * 2. GCLK (Generic Clock Controller) - Routes clocks to peripherals
 *    - Generators: Divide and select clock sources
 *    - Peripheral Channels: Connect generators to specific peripherals
 *
 * 3. MCLK (Main Clock Controller) - Controls CPU and bus clocks
 *    - Provides clock gating for peripheral buses (APB, AHB)
 *    - Configures CPU clock dividers
 *
 * Clock path: Reference -> PLL -> GCLK Generator -> GCLK Peripheral Channel -> Peripheral
 *                                      |
 *                                      v
 *                              MCLK -> CPU/Bus clocks
 */

/*
 * @brief PLL instance selection.
 *
 * PIC32CZ has two independent PLLs that can be configured separately.
 */
typedef enum whal_Pic32czClockPll_Inst{
    WHAL_PIC32CZ_PLL0,
    WHAL_PIC32CZ_PLL1,
} whal_Pic32czClockPll_Inst;

/*
 * @brief PLL reference clock source selection.
 *
 * Determines the input clock source for the PLL.
 */
typedef enum whal_Pic32czClockPll_RefSel{
    WHAL_PIC32CZ_REFSEL_GCLK,    /* Generic clock as reference */
    WHAL_PIC32CZ_REFSEL_XOSC,    /* External crystal oscillator */
    WHAL_PIC32CZ_REFSEL_DFLL48M, /* Internal 48MHz DFLL */
} whal_Pic32czClockPll_RefSel;

/*
 * @brief PLL loop filter bandwidth selection.
 *
 * Must be selected based on the PLL reference frequency after the
 * reference divider (f_ref / refDiv). Incorrect selection may cause
 * PLL instability or failure to lock.
 */
typedef enum whal_Pic32czClockPll_BwSel {
    WHAL_PIC32CZ_BWSEL_4MHz_TO_10MHz = 1,  /* Use for 4-10 MHz ref */
    WHAL_PIC32CZ_BWSEL_10MHz_TO_20MHz,     /* Use for 10-20 MHz ref */
    WHAL_PIC32CZ_BWSEL_20MHz_TO_30MHz,     /* Use for 20-30 MHz ref */
    WHAL_PIC32CZ_BWSEL_30MHz_TO_60MHz,     /* Use for 30-60 MHz ref */
} whal_Pic32czClockPll_BwSel;

/*
 * @brief GCLK generator source selection.
 *
 * Selects which clock source feeds a GCLK generator. Each PLL has
 * up to 4 independent clock outputs that can be used as sources.
 */
typedef enum whal_Pic32czClock_GenSrc {
    WHAL_PIC32CZ_GENSRC_PLL0_CLOCKOUT0 = 0x6,
    WHAL_PIC32CZ_GENSRC_PLL0_CLOCKOUT1,
    WHAL_PIC32CZ_GENSRC_PLL0_CLOCKOUT2,
    WHAL_PIC32CZ_GENSRC_PLL0_CLOCKOUT3,
} whal_Pic32czClock_GenSrc;

/* PLL output post-divider masks for PLLxPOSTDIVA register */
#define WHAL_PIC32CZ_POSTDIVMASK0 WHAL_MASK_RANGE(5, 0)   /* POSTDIV0[5:0] */
#define WHAL_PIC32CZ_POSTDIVMASK1 WHAL_MASK_RANGE(13, 8)  /* POSTDIV1[5:0] */
#define WHAL_PIC32CZ_POSTDIVMASK2 WHAL_MASK_RANGE(21, 16) /* POSTDIV2[5:0] */
#define WHAL_PIC32CZ_POSTDIVMASK3 WHAL_MASK_RANGE(29, 24) /* POSTDIV3[5:0] */

/* PLL output enable masks for PLLxPOSTDIVA register */
#define WHAL_PIC32CZ_OUTENMASK0 WHAL_MASK(7)  /* OUTEN0 */
#define WHAL_PIC32CZ_OUTENMASK1 WHAL_MASK(15) /* OUTEN1 */
#define WHAL_PIC32CZ_OUTENMASK2 WHAL_MASK(23) /* OUTEN2 */
#define WHAL_PIC32CZ_OUTENMASK3 WHAL_MASK(31) /* OUTEN3 */

/*
 * @brief PLL output configuration.
 *
 * Each PLL has up to 4 independent outputs, each with its own post-divider.
 * Output frequency = VCO frequency / postDiv
 */
typedef struct whal_Pic32czClockPll_OutCfg {
    size_t postDivMask; /* Use WHAL_PIC32CZ_POSTDIVMASKx */
    size_t outEnMask;   /* Use WHAL_PIC32CZ_OUTENMASKx */
    uint8_t postDiv;    /* Post-divider value (1-63) */
} whal_Pic32czClockPll_OutCfg;

/*
 * @brief PLL oscillator controller configuration.
 *
 * Configures a PLL within the OSCCTRL subsystem. The PLL output frequency
 * is calculated as:
 *
 *   f_vco = (f_ref / refDiv) * fbDiv
 *   f_out = f_vco / postDiv
 *
 * Where:
 *   f_ref   = Reference clock frequency (GCLK, XOSC, or DFLL48M)
 *   refDiv  = Reference divider (1-63)
 *   fbDiv   = Feedback divider (16-1023)
 *   postDiv = Per-output post-divider (1-63)
 *
 * Example: 48MHz ref, refDiv=12, fbDiv=225, postDiv=3
 *   f_vco = (48MHz / 12) * 225 = 900MHz
 *   f_out = 900MHz / 3 = 300MHz
 */
typedef struct whal_Pic32czClockPll_OscCtrlCfg {
    whal_Supply *supplyCtrl; /* Supply controller for PLL power */
    void *supply;            /* Supply instance handle */

    whal_Pic32czClockPll_Inst pllInst;   /* PLL0 or PLL1 */
    whal_Pic32czClockPll_RefSel refSel;  /* Reference clock source */
    whal_Pic32czClockPll_BwSel bwSel;    /* Loop filter bandwidth */

    uint8_t refDiv;   /* Reference divider (1-63) */
    uint16_t fbDiv;   /* Feedback divider (16-1023) */

    uint8_t outCfgCount;              /* Number of outputs to configure */
    whal_Pic32czClockPll_OutCfg *outCfg; /* Array of output configurations */
} whal_Pic32czClockPll_OscCtrlCfg;

/*
 * @brief GCLK generator configuration.
 *
 * Configures a generic clock generator. The generator takes a source clock
 * and optionally divides it before routing to peripheral channels.
 *
 *   f_gclk = f_source / genDiv
 */
typedef struct whal_Pic32czClock_GclkCfg {
    uint8_t gen;                     /* Generator number (0-11) */
    whal_Pic32czClock_GenSrc genSrc; /* Clock source for this generator */
    uint16_t genDiv;                 /* Division factor (1-65535, 0 = off) */
} whal_Pic32czClock_GclkCfg;

/*
 * @brief MCLK (Main Clock) configuration.
 *
 * Configures the main clock controller which provides clocks to the CPU
 * and peripheral buses.
 */
typedef struct whal_Pic32czClock_MclkCfg {
    uint8_t div; /* CPU clock divider */
} whal_Pic32czClock_MclkCfg;

/*
 * @brief Complete clock system configuration.
 *
 * Combines OSCCTRL, GCLK, and MCLK settings for full clock tree setup.
 */
typedef struct whal_Pic32czClock_Cfg {
    void *oscCtrlCfg;                    /* Oscillator config (PLL or other) */
    uint8_t gclkCfgCount;                /* Number of GCLK generators to configure */
    whal_Pic32czClock_GclkCfg *gclkCfg;  /* Array of generator configurations */
    whal_Pic32czClock_MclkCfg *mclkCfg;  /* Main clock configuration */
} whal_Pic32czClock_Cfg;

/*
 * @brief Peripheral clock descriptor.
 *
 * Describes how to enable clocking for a specific peripheral. Each peripheral
 * needs both a GCLK peripheral channel connection and an MCLK bus enable.
 *
 * The gclkPeriphChannel and gclkPeriphSrc connect the peripheral to a GCLK
 * generator. The mclkEnableInst and mclkEnableMask enable the peripheral's
 * bus clock in the MCLK controller.
 *
 * These values are found in the datasheet's "Peripheral Clock Masking" and
 * "GCLK Peripheral Channel Mapping" tables.
 */
typedef struct whal_Pic32czClock_Clk {
    size_t gclkPeriphChannel; /* GCLK peripheral channel index */
    uint8_t gclkPeriphSrc;    /* GCLK generator to use as source */
    size_t mclkEnableInst;    /* MCLK mask register instance */
    size_t mclkEnableMask;    /* Bit mask within the MCLK mask register */
} whal_Pic32czClock_Clk;

whal_Error WHAL_DRV_FN(Pic32czClockPll, init)(whal_Clock *clkDev);
whal_Error WHAL_DRV_FN(Pic32czClockPll, deinit)(whal_Clock *clkDev);
whal_Error WHAL_DRV_FN(Pic32czClockPll, enable)(whal_Clock *clkDev, const void *clk);
whal_Error WHAL_DRV_FN(Pic32czClockPll, disable)(whal_Clock *clkDev, const void *clk);
whal_Error WHAL_DRV_FN(Pic32czClockPll, getrate)(whal_Clock *clkDev, size_t *rateOut);

#endif /* WHAL_PIC32CZ_CLOCK_H */
