/* stm32wb_lptim_pwm.h
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

#ifndef WHAL_STM32WB_LPTIM_PWM_H
#define WHAL_STM32WB_LPTIM_PWM_H

#include <stdint.h>
#include <stddef.h>
#include <wolfHAL/pwm/pwm.h>
#include <wolfHAL/timeout.h>

/*
 * @file stm32wb_lptim_pwm.h
 * @brief STM32WB LPTIM-based PWM driver configuration.
 *
 * Implements the generic whal_Pwm interface on top of an STM32WB
 * low-power timer (LPTIM). The LPTIM has a single PWM output, so the only
 * valid channel is WHAL_STM32WB_LPTIM_PWM_CHANNEL (0). The waveform is
 * passed to Start as a whal_Pwm_ChannelCfg, which programs it and begins
 * output; Stop halts it. The driver configuration holds the prescaler, clock
 * source, and polling timeout.
 *
 * The LPTIM has no pulse counter, so only continuous output is supported:
 * the waveform's pulseCount must be WHAL_PWM_PULSE_COUNT_CONTINUOUS, and any
 * finite count is rejected with WHAL_ENOTSUP.
 *
 * The board is responsible for enabling the LPTIM kernel clock and
 * selecting its source (LSE/LSI/HSI16/PCLK) via RCC before whal_Pwm_Init
 * is called; this driver only touches LPTIM registers. The period and
 * pulse are counted in ticks of that kernel clock divided by the
 * configured prescaler.
 */

/* The LPTIM exposes a single PWM output. */
#define WHAL_STM32WB_LPTIM_PWM_CHANNEL  0

/* Clock source for LPTIM_CFGR.CKSEL. */
#define WHAL_STM32WB_LPTIM_PWM_CLKSEL_INTERNAL  0  /* RCC-supplied kernel clock */
#define WHAL_STM32WB_LPTIM_PWM_CLKSEL_EXTERNAL  1  /* external clock on LPTIM input */

/*
 * @brief STM32WB LPTIM PWM device configuration.
 */
typedef struct whal_Stm32wb_Lptim_Pwm_Cfg {
    /* Prescaler field value (LPTIM_CFGR.PRESC[2:0]): 0 = /1, 1 = /2, 2 = /4,
     * ... 7 = /128. Divides the LPTIM kernel clock to form the PWM tick that
     * the periodCycles / pulseCycles passed to Start are measured in. */
    uint8_t prescaler;
    /* Clock source (LPTIM_CFGR.CKSEL): WHAL_STM32WB_LPTIM_PWM_CLKSEL_INTERNAL
     * counts the RCC-supplied kernel clock; _EXTERNAL counts a clock applied
     * on the LPTIM input. */
    uint8_t clkSel;
    /* Timeout used while polling the ARROK / CMPOK update-synchronization
     * flags after writing the autoreload and compare registers. */
    whal_Timeout *timeout;
} whal_Stm32wb_Lptim_Pwm_Cfg;

/*
 * @brief Driver instance for the STM32WB LPTIM PWM peripheral.
 */
extern const whal_PwmDriver whal_Stm32wb_Lptim_Pwm_Driver;

/*
 * @brief Initialize the LPTIM and apply the instance configuration.
 *
 * Configures the instance-static LPTIM settings (clock source, prescaler,
 * PWM waveform) with the timer disabled. The per-call polarity
 * and the ARR/CMP waveform are programmed by whal_Pwm_Start. The board must
 * have enabled the LPTIM kernel clock beforehand.
 *
 * @param dev Pointer to the PWM instance to initialize.
 *
 * @retval WHAL_SUCCESS Initialization completed.
 * @retval WHAL_EINVAL  Null pointer or missing configuration.
 * @retval WHAL_ENOTSUP Prescaler or clock source out of range.
 */
whal_Error whal_Stm32wb_Lptim_Pwm_Init(whal_Pwm *dev);
/*
 * @brief Deinitialize the LPTIM PWM output.
 *
 * @param dev Pointer to the PWM instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Deinitialization completed.
 * @retval WHAL_EINVAL  Null pointer.
 */
whal_Error whal_Stm32wb_Lptim_Pwm_Deinit(whal_Pwm *dev);
/*
 * @brief Program the given waveform and start PWM output.
 *
 * Sets the output polarity and programs LPTIM_ARR / LPTIM_CMP from @p cfg's
 * period and pulse (waiting for the ARROK / CMPOK synchronization flags),
 * then sets LPTIM_CR.CNTSTRT to begin continuous counting. The clock source,
 * prescaler, and PWM waveform mode are configured by Init.
 *
 * Because the LPTIM asserts its output while LPTIM_CNT exceeds LPTIM_CMP
 * (and clears it on the ARR match), the compare value is derived as
 * (ARR - pulseCycles). ARR must exceed CMP, so pulseCycles must be between 1
 * and periodCycles - 1.
 *
 * @param dev     Pointer to the PWM instance.
 * @param channel Must be WHAL_STM32WB_LPTIM_PWM_CHANNEL (0).
 * @param cfg     Waveform (period, pulse, polarity) to apply.
 *
 * @retval WHAL_SUCCESS Output started.
 * @retval WHAL_EINVAL  Null pointer, missing configuration, zero period, or
 *                      pulse exceeding period.
 * @retval WHAL_ENOTSUP Unsupported channel, period out of the 16-bit range or
 *                      below 2, pulse of 0 or equal to the period, or a
 *                      non-continuous pulse count.
 * @retval WHAL_ETIMEOUT ARR/CMP update did not synchronize before timeout.
 */
whal_Error whal_Stm32wb_Lptim_Pwm_Start(whal_Pwm *dev, uint8_t channel,
                                        const whal_Pwm_ChannelCfg *cfg);
/*
 * @brief Stop the PWM output.
 *
 * Clears LPTIM_CR.ENABLE, which halts the counter and parks the output.
 * whal_Pwm_Start resumes output without a re-Init; the Init-configured
 * clock source, prescaler, and waveform mode are retained.
 *
 * @param dev     Pointer to the PWM instance.
 * @param channel Must be WHAL_STM32WB_LPTIM_PWM_CHANNEL (0).
 *
 * @retval WHAL_SUCCESS Output stopped.
 * @retval WHAL_EINVAL  Null pointer.
 * @retval WHAL_ENOTSUP Unsupported channel.
 */
whal_Error whal_Stm32wb_Lptim_Pwm_Stop(whal_Pwm *dev, uint8_t channel);

#endif /* WHAL_STM32WB_LPTIM_PWM_H */
