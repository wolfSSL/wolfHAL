/* pic32cz_clock.h
 *
 * Copyright (C) 2026 wolfSSL Inc.
 *
 * This file is part of wolfHAL.
 *
 * wolfHAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfHAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#ifndef WHAL_PIC32CZ_CLOCK_H
#define WHAL_PIC32CZ_CLOCK_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/error.h>
#include <wolfHAL/reg.h>
#include <wolfHAL/bitops.h>

/*
 * @file pic32cz_clock.h
 * @brief PIC32CZ clock system driver.
 *
 * Boards bring up the clock tree imperatively from Board_Init.
 *
 * The PIC32CZ clock subsystems:
 *   OSCCTRL — PLL0/PLL1 fractional PLLs (each up to 4 outputs).
 *   GCLK    — N generic clock generators routed to peripheral channels.
 *   MCLK    — main clock controller (CPU divider + peripheral bus gates).
 *
 * Path:  Reference -> PLL -> GCLK Generator -> GCLK Peripheral Channel -> Peripheral
 *                                            -> MCLK -> CPU/bus clocks
 */

#define WHAL_PIC32CZ_CLOCK_BASE            0x44040000

/*
 * PIC32CZ Clock System Register Map.
 * OSCCTRL @ 0x00000, GCLK @ 0x10000, MCLK @ 0x12000.
 */

#define WHAL_PIC32CZ_CLOCK_OSCCTRL 0x00000

#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_REG(pllInst)        (0x40 + ((pllInst) * 20))
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_ENABLE_Pos          1
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_ENABLE_Msk          (1UL << WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_ENABLE_Pos)
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_REFSEL_Pos          8
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_REFSEL_Msk          (WHAL_BITMASK(3) << WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_REFSEL_Pos)
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_BWSEL_Pos           11
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_BWSEL_Msk           (WHAL_BITMASK(3) << WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_BWSEL_Pos)

#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxFBDIV_REG(pllInst)       (0x44 + ((pllInst) * 20))
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxFBDIV_Pos                0
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxFBDIV_Msk                (WHAL_BITMASK(10) << WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxFBDIV_Pos)

#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxREFDIV_REG(pllInst)      (0x48 + ((pllInst) * 20))
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxREFDIV_Pos               0
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxREFDIV_Msk               (WHAL_BITMASK(6) << WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxREFDIV_Pos)

#define WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxPOSTDIVA_REG(pllInst)    (0x4C + ((pllInst) * 20))

#define WHAL_PIC32CZ_CLOCK_OSCCTRL_STATUS_REG                   (WHAL_PIC32CZ_CLOCK_OSCCTRL + 0x10)
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_STATUS_PLLxLOCK_Pos(pllInst) (24 + (pllInst))
#define WHAL_PIC32CZ_CLOCK_OSCCTRL_STATUS_PLLxLOCK_Msk(pllInst) (1UL << WHAL_PIC32CZ_CLOCK_OSCCTRL_STATUS_PLLxLOCK_Pos(pllInst))

#define WHAL_PIC32CZ_CLOCK_GCLK 0x10000

#define WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_REG(gclkInst) ((WHAL_PIC32CZ_CLOCK_GCLK + 0x20 + ((gclkInst) * 0x4)))
#define WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_SRC_Pos       0
#define WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_SRC_Msk       (WHAL_BITMASK(5) << WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_SRC_Pos)
#define WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_GENEN_Pos     8
#define WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_GENEN_Msk     (1UL << WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_GENEN_Pos)
#define WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_DIV_Pos       16
#define WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_DIV_Msk       (WHAL_BITMASK(16) << WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_DIV_Pos)

#define WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_REG(periphChannel) (WHAL_PIC32CZ_CLOCK_GCLK + 0x80 + ((periphChannel) * 0x4))
#define WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_GEN_Pos            0
#define WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_GEN_Msk            (WHAL_BITMASK(4) << WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_GEN_Pos)
#define WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_CHEN_Pos           6
#define WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_CHEN_Msk           (1UL << WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_CHEN_Pos)

#define WHAL_PIC32CZ_CLOCK_GCLK_SYNCBUSY_REG                    (WHAL_PIC32CZ_CLOCK_GCLK + 0x04)
#define WHAL_PIC32CZ_CLOCK_GCLK_SYNCBUSY_GENCTRLx_Pos(gclkInst) (2 + (gclkInst))
#define WHAL_PIC32CZ_CLOCK_GCLK_SYNCBUSY_GENCTRLx_Msk(gclkInst) (1UL << WHAL_PIC32CZ_CLOCK_GCLK_SYNCBUSY_GENCTRLx_Pos(gclkInst))

#define WHAL_PIC32CZ_CLOCK_MCLK 0x12000
#define WHAL_PIC32CZ_CLOCK_MCLK_INTFLAG_REG       (WHAL_PIC32CZ_CLOCK_MCLK + 0x08)
#define WHAL_PIC32CZ_CLOCK_MCLK_INTFLAG_CKRDY_Pos 0
#define WHAL_PIC32CZ_CLOCK_MCLK_INTFLAG_CKRDY_Msk (1UL << WHAL_PIC32CZ_CLOCK_MCLK_INTFLAG_CKRDY_Pos)
#define WHAL_PIC32CZ_CLOCK_MCLK_DIV1_REG          (WHAL_PIC32CZ_CLOCK_MCLK + 0x10)
#define WHAL_PIC32CZ_CLOCK_MCLK_DIV1_Pos          0
#define WHAL_PIC32CZ_CLOCK_MCLK_DIV1_Msk          (WHAL_BITMASK(8) << WHAL_PIC32CZ_CLOCK_MCLK_DIV1_Pos)
#define WHAL_PIC32CZ_CLOCK_MCLK_CLKxMSK_REG(enableInst) (WHAL_PIC32CZ_CLOCK_MCLK + 0x3C + ((enableInst) * 0x4))

/*
 * @brief PLL instance.
 */
typedef enum {
    WHAL_PIC32CZ_PLL0,
    WHAL_PIC32CZ_PLL1,
} whal_Pic32cz_Clock_PllInst;

/*
 * @brief PLL reference clock source selection.
 */
typedef enum {
    WHAL_PIC32CZ_REFSEL_GCLK,
    WHAL_PIC32CZ_REFSEL_XOSC,
    WHAL_PIC32CZ_REFSEL_DFLL48M,
} whal_Pic32cz_Clock_PllRefSel;

/*
 * @brief PLL loop filter bandwidth selection.
 */
typedef enum {
    WHAL_PIC32CZ_BWSEL_4MHz_TO_10MHz  = 1,
    WHAL_PIC32CZ_BWSEL_10MHz_TO_20MHz,
    WHAL_PIC32CZ_BWSEL_20MHz_TO_30MHz,
    WHAL_PIC32CZ_BWSEL_30MHz_TO_60MHz,
} whal_Pic32cz_Clock_PllBwSel;

/*
 * @brief GCLK generator source selection.
 */
typedef enum {
    WHAL_PIC32CZ_GENSRC_PLL0_CLOCKOUT0 = 0x6,
    WHAL_PIC32CZ_GENSRC_PLL0_CLOCKOUT1,
    WHAL_PIC32CZ_GENSRC_PLL0_CLOCKOUT2,
    WHAL_PIC32CZ_GENSRC_PLL0_CLOCKOUT3,
} whal_Pic32cz_Clock_GenSrc;

/* PLL output post-divider field positions for PLLxPOSTDIVA */
#define WHAL_PIC32CZ_POSTDIV0_Pos (0)
#define WHAL_PIC32CZ_POSTDIV0_Msk (WHAL_BITMASK(6) << 0)
#define WHAL_PIC32CZ_POSTDIV1_Pos (8)
#define WHAL_PIC32CZ_POSTDIV1_Msk (WHAL_BITMASK(6) << 8)
#define WHAL_PIC32CZ_POSTDIV2_Pos (16)
#define WHAL_PIC32CZ_POSTDIV2_Msk (WHAL_BITMASK(6) << 16)
#define WHAL_PIC32CZ_POSTDIV3_Pos (24)
#define WHAL_PIC32CZ_POSTDIV3_Msk (WHAL_BITMASK(6) << 24)
#define WHAL_PIC32CZ_OUTEN0_Pos (7)
#define WHAL_PIC32CZ_OUTEN0_Msk (1UL << 7)
#define WHAL_PIC32CZ_OUTEN1_Pos (15)
#define WHAL_PIC32CZ_OUTEN1_Msk (1UL << 15)
#define WHAL_PIC32CZ_OUTEN2_Pos (23)
#define WHAL_PIC32CZ_OUTEN2_Msk (1UL << 23)
#define WHAL_PIC32CZ_OUTEN3_Pos (31)
#define WHAL_PIC32CZ_OUTEN3_Msk (1UL << 31)

/*
 * @brief One PLL output configuration. f_out = f_vco / postDiv.
 */
typedef struct {
    size_t postDivMask;
    size_t postDivPos;
    size_t outEnMask;
    size_t outEnPos;
    uint8_t postDiv;
} whal_Pic32cz_Clock_PllOutCfg;

/*
 * @brief PLL configuration.
 *   f_vco = (f_ref / refDiv) * fbDiv
 *   refDiv: 1-63
 *   fbDiv:  16-1023
 */
typedef struct {
    whal_Pic32cz_Clock_PllInst pllInst;
    whal_Pic32cz_Clock_PllRefSel refSel;
    whal_Pic32cz_Clock_PllBwSel bwSel;
    uint8_t refDiv;
    uint16_t fbDiv;
    uint8_t outCfgCount;
    const whal_Pic32cz_Clock_PllOutCfg *outCfg;
} whal_Pic32cz_Clock_PllCfg;

/*
 * @brief GCLK generator configuration.
 *   gen:    generator index (0-11)
 *   genSrc: clock source for this generator
 *   genDiv: divider (1-65535, 0 = off)
 */
typedef struct {
    uint8_t gen;
    whal_Pic32cz_Clock_GenSrc genSrc;
    uint16_t genDiv;
} whal_Pic32cz_Clock_GenCfg;

/*
 * @brief Peripheral clock descriptor (GCLK channel + MCLK enable bit).
 */
typedef struct {
    size_t gclkPeriphChannel;
    uint8_t gclkPeriphSrc;
    size_t mclkEnableInst;
    size_t mclkEnableMask;
    size_t mclkEnablePos;
} whal_Pic32cz_Clock_PeriphClk;

/*
 * @brief Configure and enable a PLL, then wait for PLLxLOCK.
 *
 * Programs PLLxFBDIV, PLLxREFDIV, the requested output post-dividers /
 * output-enable bits in PLLxPOSTDIVA, then enables the PLL by setting
 * ENABLE/REFSEL/BWSEL in PLLxCTRL. Polls OSCCTRL.STATUS until the
 * matching PLLxLOCK bit asserts.
 *
 * @param cfg    PLL configuration.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Pic32cz_Clock_EnablePll(
    const whal_Pic32cz_Clock_PllCfg *cfg)
{
    uint32_t status;
    uint8_t i;
    size_t pllCtrlReg, pllFbdivReg, pllRefdivReg, pllPostdivaReg;

    pllCtrlReg     = WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_REG(cfg->pllInst);
    pllFbdivReg    = WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxFBDIV_REG(cfg->pllInst);
    pllRefdivReg   = WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxREFDIV_REG(cfg->pllInst);
    pllPostdivaReg = WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxPOSTDIVA_REG(cfg->pllInst);

    whal_Reg32_Update(WHAL_PIC32CZ_CLOCK_BASE, pllFbdivReg,
                      WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxFBDIV_Msk,
                      whal_SetBits(WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxFBDIV_Msk,
                                   WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxFBDIV_Pos,
                                   cfg->fbDiv));
    whal_Reg32_Update(WHAL_PIC32CZ_CLOCK_BASE, pllRefdivReg,
                      WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxREFDIV_Msk,
                      whal_SetBits(WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxREFDIV_Msk,
                                   WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxREFDIV_Pos,
                                   cfg->refDiv));
    for (i = 0; i < cfg->outCfgCount; i++) {
        const whal_Pic32cz_Clock_PllOutCfg *out = &cfg->outCfg[i];
        whal_Reg32_Update(WHAL_PIC32CZ_CLOCK_BASE, pllPostdivaReg,
                          out->outEnMask | out->postDivMask,
                          whal_SetBits(out->postDivMask, out->postDivPos, out->postDiv) |
                          whal_SetBits(out->outEnMask, out->outEnPos, 1));
    }

    whal_Reg32_Update(WHAL_PIC32CZ_CLOCK_BASE, pllCtrlReg,
                      WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_ENABLE_Msk |
                      WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_REFSEL_Msk |
                      WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_BWSEL_Msk,
                      WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_ENABLE_Msk |
                      whal_SetBits(WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_REFSEL_Msk,
                                   WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_REFSEL_Pos, cfg->refSel) |
                      whal_SetBits(WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_BWSEL_Msk,
                                   WHAL_PIC32CZ_CLOCK_OSCCTRL_PLLxCTRL_BWSEL_Pos, cfg->bwSel));

    do {
        whal_Reg32_Get(WHAL_PIC32CZ_CLOCK_BASE, WHAL_PIC32CZ_CLOCK_OSCCTRL_STATUS_REG,
                       WHAL_PIC32CZ_CLOCK_OSCCTRL_STATUS_PLLxLOCK_Msk(cfg->pllInst),
                       WHAL_PIC32CZ_CLOCK_OSCCTRL_STATUS_PLLxLOCK_Pos(cfg->pllInst),
                       &status);
    } while (!status);
    return WHAL_SUCCESS;
}

/*
 * @brief Enable a GCLK generator with the supplied source and divider.
 *
 * Programs GCLK_GENCTRLx (SRC/GENEN/DIV) and waits for the GCLK_SYNCBUSY
 * bit for this generator to clear.
 *
 * @param cfg    Generator configuration.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Pic32cz_Clock_EnableGclkGen(
    const whal_Pic32cz_Clock_GenCfg *cfg)
{
    uint32_t status;

    whal_Reg32_Update(WHAL_PIC32CZ_CLOCK_BASE, WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_REG(cfg->gen),
                      WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_SRC_Msk |
                      WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_GENEN_Msk |
                      WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_DIV_Msk,
                      whal_SetBits(WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_SRC_Msk,
                                   WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_SRC_Pos, cfg->genSrc) |
                      WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_GENEN_Msk |
                      whal_SetBits(WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_DIV_Msk,
                                   WHAL_PIC32CZ_CLOCK_GCLK_GENCTRLx_DIV_Pos, cfg->genDiv));
    do {
        whal_Reg32_Get(WHAL_PIC32CZ_CLOCK_BASE, WHAL_PIC32CZ_CLOCK_GCLK_SYNCBUSY_REG,
                       WHAL_PIC32CZ_CLOCK_GCLK_SYNCBUSY_GENCTRLx_Msk(cfg->gen),
                       WHAL_PIC32CZ_CLOCK_GCLK_SYNCBUSY_GENCTRLx_Pos(cfg->gen),
                       &status);
    } while (status);
    return WHAL_SUCCESS;
}

/*
 * @brief Program the main clock CPU divider (MCLK DIV1).
 *
 * Writes MCLK.DIV1 and polls MCLK.INTFLAG.CKRDY for the change to settle.
 *
 * @param div    Divider value to write into MCLK.DIV1.
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Pic32cz_Clock_SetMclkDiv(
    uint8_t div)
{
    uint32_t status;

    whal_Reg32_Update(WHAL_PIC32CZ_CLOCK_BASE, WHAL_PIC32CZ_CLOCK_MCLK_DIV1_REG,
                      WHAL_PIC32CZ_CLOCK_MCLK_DIV1_Msk,
                      whal_SetBits(WHAL_PIC32CZ_CLOCK_MCLK_DIV1_Msk,
                                   WHAL_PIC32CZ_CLOCK_MCLK_DIV1_Pos, div));
    do {
        whal_Reg32_Get(WHAL_PIC32CZ_CLOCK_BASE, WHAL_PIC32CZ_CLOCK_MCLK_INTFLAG_REG,
                       WHAL_PIC32CZ_CLOCK_MCLK_INTFLAG_CKRDY_Msk,
                       WHAL_PIC32CZ_CLOCK_MCLK_INTFLAG_CKRDY_Pos, &status);
    } while (!status);
    return WHAL_SUCCESS;
}

/*
 * @brief Route a GCLK generator to a peripheral channel and ungate the MCLK bit.
 *
 * Configures GCLK_PCHCTRLx with the requested generator + channel-enable
 * bit, then sets the matching MCLK_CLKxMSK enable bit.
 *
 * @param clk    Peripheral clock descriptor (GCLK channel + MCLK enable bit).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Pic32cz_Clock_EnablePeriphClk(
    const whal_Pic32cz_Clock_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_PIC32CZ_CLOCK_BASE,
                      WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_REG(clk->gclkPeriphChannel),
                      WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_GEN_Msk |
                      WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_CHEN_Msk,
                      whal_SetBits(WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_GEN_Msk,
                                   WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_GEN_Pos,
                                   clk->gclkPeriphSrc) |
                      WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_CHEN_Msk);
    whal_Reg32_Update(WHAL_PIC32CZ_CLOCK_BASE,
                      WHAL_PIC32CZ_CLOCK_MCLK_CLKxMSK_REG(clk->mclkEnableInst),
                      clk->mclkEnableMask,
                      whal_SetBits(clk->mclkEnableMask, clk->mclkEnablePos, 1));
    return WHAL_SUCCESS;
}

/*
 * @brief Gate off a peripheral's MCLK bit and disable the GCLK channel.
 *
 * @param clk    Peripheral clock descriptor (GCLK channel + MCLK enable bit).
 *
 * @retval WHAL_SUCCESS Always.
 */
static inline whal_Error whal_Pic32cz_Clock_DisablePeriphClk(
    const whal_Pic32cz_Clock_PeriphClk *clk)
{
    whal_Reg32_Update(WHAL_PIC32CZ_CLOCK_BASE,
                      WHAL_PIC32CZ_CLOCK_MCLK_CLKxMSK_REG(clk->mclkEnableInst),
                      clk->mclkEnableMask,
                      whal_SetBits(clk->mclkEnableMask, clk->mclkEnablePos, 0));
    whal_Reg32_Update(WHAL_PIC32CZ_CLOCK_BASE,
                      WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_REG(clk->gclkPeriphChannel),
                      WHAL_PIC32CZ_CLOCK_GCLK_PCHCTRLx_CHEN_Msk, 0);
    return WHAL_SUCCESS;
}

#endif /* WHAL_PIC32CZ_CLOCK_H */
