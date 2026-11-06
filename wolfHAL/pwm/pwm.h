/* pwm.h
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

#ifndef WHAL_PWM_H
#define WHAL_PWM_H

#include <wolfHAL/error.h>
#include <stdint.h>
#include <stddef.h>

/*
 * @file pwm.h
 * @brief Generic PWM abstraction and driver interface.
 *
 * Provides a hardware-agnostic API for generating pulse-width-modulated
 * output. The waveform of a channel (frequency, duty, polarity) is described
 * by a whal_Pwm_ChannelCfg passed to Start, which applies it and begins
 * output; Stop halts the channel.
 *
 * Channels are addressed by a zero-based index. A driver returns
 * WHAL_ENOTSUP for any channel its hardware does not implement; a
 * single-output peripheral accepts only channel 0.
 */

/* Output polarity for a PWM channel. */
#define WHAL_PWM_POLARITY_NORMAL    0  /* pulse drives the active-high level */
#define WHAL_PWM_POLARITY_INVERTED  1  /* pulse drives the active-low level */

/* Run the channel continuously with no automatic stop; value for
 * whal_Pwm_ChannelCfg.pulseCount. Any nonzero value requests exactly that
 * many pulses; drivers that cannot count pulses reject it with WHAL_ENOTSUP. */
#define WHAL_PWM_PULSE_COUNT_CONTINUOUS  0

/*
 * @brief Generic description of a single PWM channel's waveform.
 *
 * periodCycles sets the frequency, pulseCycles the duty, polarity the active
 * level, and pulseCount whether the output runs continuously or stops after a
 * fixed number of pulses. The cycle counts are in ticks of the driver's
 * (prescaled) timer clock. On hardware that shares one counter across
 * channels, every channel of an instance must use the same periodCycles.
 */
typedef struct whal_Pwm_ChannelCfg {
#ifdef WHAL_CFG_64BIT_TICK
    uint64_t periodCycles;  /* full cycle length in timer ticks (1..) */
    uint64_t pulseCycles;   /* asserted width in ticks; must be <= periodCycles */
#else
    uint32_t periodCycles;  /* full cycle length in timer ticks (1..) */
    uint32_t pulseCycles;   /* asserted width in ticks; must be <= periodCycles */
#endif
    uint32_t pulseCount;    /* WHAL_PWM_PULSE_COUNT_CONTINUOUS or number of pulses */
    uint8_t  polarity;      /* WHAL_PWM_POLARITY_NORMAL or _INVERTED */
} whal_Pwm_ChannelCfg;

typedef struct whal_Pwm whal_Pwm;

/*
 * @brief Driver vtable for PWM devices.
 */
typedef struct {
    /* Initialize the PWM hardware. */
    whal_Error (*Init)(whal_Pwm *dev);
    /* Deinitialize the PWM hardware. */
    whal_Error (*Deinit)(whal_Pwm *dev);
    /* Apply the given waveform to a channel and start its output. */
    whal_Error (*Start)(whal_Pwm *dev, uint8_t channel,
                        const whal_Pwm_ChannelCfg *cfg);
    /* Stop output on a channel. */
    whal_Error (*Stop)(whal_Pwm *dev, uint8_t channel);
} whal_PwmDriver;

/*
 * @brief PWM device instance tying a driver and configuration.
 */
struct whal_Pwm {
    const uintptr_t base;
    const whal_PwmDriver *driver;
    void *cfg;
};

/*
 * @brief Initialize a PWM device.
 *
 * @param dev Pointer to the PWM instance to initialize.
 *
 * @retval WHAL_SUCCESS Driver-specific init completed.
 * @retval WHAL_EINVAL  Null pointer.
 * @retval WHAL_ENOTSUP No driver bound or operation unsupported.
 */
whal_Error whal_Pwm_Init(whal_Pwm *dev);
/*
 * @brief Deinitialize a PWM device and release resources.
 *
 * @param dev Pointer to the PWM instance to deinitialize.
 *
 * @retval WHAL_SUCCESS Driver-specific deinit completed.
 * @retval WHAL_EINVAL  Null pointer.
 * @retval WHAL_ENOTSUP No driver bound or operation unsupported.
 */
whal_Error whal_Pwm_Deinit(whal_Pwm *dev);
/*
 * @brief Apply a waveform to a channel and start its output.
 *
 * @param dev     Pointer to the PWM instance.
 * @param channel Zero-based channel to start.
 * @param cfg     Waveform (period, pulse, polarity) to apply.
 *
 * @retval WHAL_SUCCESS Output started.
 * @retval WHAL_EINVAL  Null pointer, zero period, pulse exceeds period, or
 *                      undefined polarity.
 * @retval WHAL_ENOTSUP No driver bound, or channel/waveform unsupported.
 */
whal_Error whal_Pwm_Start(whal_Pwm *dev, uint8_t channel,
                          const whal_Pwm_ChannelCfg *cfg);
/*
 * @brief Stop PWM output on a channel.
 *
 * @param dev     Pointer to the PWM instance.
 * @param channel Zero-based channel to stop.
 *
 * @retval WHAL_SUCCESS Output stopped.
 * @retval WHAL_EINVAL  Null pointer.
 * @retval WHAL_ENOTSUP No driver bound, or channel unsupported.
 */
whal_Error whal_Pwm_Stop(whal_Pwm *dev, uint8_t channel);

#endif /* WHAL_PWM_H */
