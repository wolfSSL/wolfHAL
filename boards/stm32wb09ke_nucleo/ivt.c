/* ivt.c
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

#include <stdint.h>
#include <stddef.h>

extern uint32_t _estack[];
extern uint32_t _sidata[];
extern uint32_t _sdata[];
extern uint32_t _edata[];
extern uint32_t _sbss[];
extern uint32_t _ebss[];

extern void main(void);

void __attribute__((naked, noreturn)) Default_Handler(void)
{
    while (1);
}

void Reset_Handler(void) __attribute__((weak));
void NMI_Handler(void)        __attribute__((weak, noreturn, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, noreturn, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, noreturn, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, noreturn, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, noreturn, alias("Default_Handler")));

/* STM32WB09xE peripheral interrupts (RM0505 Table 6) */
void NVM_IRQHandler(void)       __attribute__((weak, noreturn, alias("Default_Handler")));
void RCC_IRQHandler(void)       __attribute__((weak, noreturn, alias("Default_Handler")));
void BATTERY_IRQHandler(void)   __attribute__((weak, noreturn, alias("Default_Handler")));
void I2C1_IRQHandler(void)      __attribute__((weak, noreturn, alias("Default_Handler")));
void SPI3_IRQHandler(void)      __attribute__((weak, noreturn, alias("Default_Handler")));
void USART_IRQHandler(void)     __attribute__((weak, noreturn, alias("Default_Handler")));
void LPUART_IRQHandler(void)    __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM2_IRQHandler(void)      __attribute__((weak, noreturn, alias("Default_Handler")));
void RTC_IRQHandler(void)       __attribute__((weak, noreturn, alias("Default_Handler")));
void ADC_IRQHandler(void)       __attribute__((weak, noreturn, alias("Default_Handler")));
void PKA_IRQHandler(void)       __attribute__((weak, noreturn, alias("Default_Handler")));
void GPIOA_IRQHandler(void)     __attribute__((weak, noreturn, alias("Default_Handler")));
void GPIOB_IRQHandler(void)     __attribute__((weak, noreturn, alias("Default_Handler")));
void DMA_IRQHandler(void)       __attribute__((weak, noreturn, alias("Default_Handler")));
void BLE_TXRX_IRQHandler(void)  __attribute__((weak, noreturn, alias("Default_Handler")));
void RADIO_CTRL_IRQHandler(void) __attribute__((weak, noreturn, alias("Default_Handler")));
void MR_BLE_IRQHandler(void)    __attribute__((weak, noreturn, alias("Default_Handler")));
void CPU_WKUP_IRQHandler(void)  __attribute__((weak, noreturn, alias("Default_Handler")));
void BLE_WKUP_IRQHandler(void)  __attribute__((weak, noreturn, alias("Default_Handler")));
void BLE_SEQ_IRQHandler(void)   __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM16_IRQHandler(void)     __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM17_IRQHandler(void)     __attribute__((weak, noreturn, alias("Default_Handler")));
void TRNG_IRQHandler(void)      __attribute__((weak, noreturn, alias("Default_Handler")));

#define RESERVED Default_Handler

void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;

    for (size_t i = 0; i < n; i++)
        d[i] = s[i];

    return dest;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = s;
    unsigned char v = (unsigned char)c;

    for (size_t i = 0; i < n; i++)
        p[i] = v;

    return s;
}

void (* const interrupt_vector_table[])(void) __attribute__((section(".isr_vector"))) = {
    (void (*)(void))_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    RESERVED,                                  /* Reserved */
    RESERVED,                                  /* Reserved */
    RESERVED,                                  /* Reserved */
    RESERVED,                                  /* Reserved */
    RESERVED,                                  /* Reserved */
    RESERVED,                                  /* Reserved */
    RESERVED,                                  /* Reserved */
    SVC_Handler,
    RESERVED,                                  /* Reserved */
    RESERVED,                                  /* Reserved */
    PendSV_Handler,
    SysTick_Handler,
    /* STM32WB09xE peripheral interrupts (Table 6) */
    NVM_IRQHandler,                            /* 0  Flash controller */
    RCC_IRQHandler,                            /* 1  Reset and clock controller */
    BATTERY_IRQHandler,                        /* 2  PVD */
    I2C1_IRQHandler,                           /* 3  I2C1 */
    RESERVED,                                  /* 4  Reserved */
    RESERVED,                                  /* 5  Reserved */
    RESERVED,                                  /* 6  Reserved */
    SPI3_IRQHandler,                           /* 7  SPI3 */
    USART_IRQHandler,                          /* 8  USART */
    LPUART_IRQHandler,                         /* 9  LPUART */
    TIM2_IRQHandler,                           /* 10 TIM2 */
    RTC_IRQHandler,                            /* 11 RTC */
    ADC_IRQHandler,                            /* 12 ADC (shared with reserved slot in TRM) */
    PKA_IRQHandler,                            /* 13 PKA */
    RESERVED,                                  /* 14 Reserved */
    GPIOA_IRQHandler,                          /* 15 GPIOA */
    GPIOB_IRQHandler,                          /* 16 GPIOB */
    DMA_IRQHandler,                            /* 17 DMA */
    BLE_TXRX_IRQHandler,                       /* 18 BLE end-of-transfer */
    RESERVED,                                  /* 19 Reserved */
    RESERVED,                                  /* 20 Reserved */
    RADIO_CTRL_IRQHandler,                     /* 21 Radio control slow-clock measurement */
    MR_BLE_IRQHandler,                         /* 22 RRM and radio FSM */
    CPU_WKUP_IRQHandler,                       /* 23 CPU wakeup */
    BLE_WKUP_IRQHandler,                       /* 24 BLE wakeup */
    BLE_SEQ_IRQHandler,                        /* 25 BLE RX/TX sequence */
    TIM16_IRQHandler,                          /* 26 TIM16 */
    TIM17_IRQHandler,                          /* 27 TIM17 */
    TRNG_IRQHandler,                           /* 28 TRNG */
    RESERVED,                                  /* 29 Reserved */
    RESERVED,                                  /* 30 Reserved */
    RESERVED,                                  /* 31 Reserved */
};

void __attribute__((naked)) Reset_Handler(void)
{
    __asm__("ldr r0, =_estack\n\t"
            "mov sp, r0");

    /* Copy data section from flash to RAM */
    uint32_t data_section_size = _edata - _sdata;
    memcpy(_sdata, _sidata, data_section_size * 4);

    /* Zero out bss */
    uint32_t bss_section_size = _ebss - _sbss;
    memset(_sbss, 0, bss_section_size * 4);

    /* Set Interrupt Vector Table Offset (Cortex-M0+ VTOR at 0xE000ED08). */
    uint32_t *vtor = (uint32_t *)0xE000ED08;
    *vtor = (uint32_t)interrupt_vector_table;

    /* WB0 bootROM hands control to user code with PRIMASK set. Clear it
     * so SysTick and other exceptions can be delivered. */
    __asm__ volatile ("cpsie i" ::: "memory");

    main();
}
