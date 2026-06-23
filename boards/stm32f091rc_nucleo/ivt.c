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

extern void main();

void __attribute__((naked,noreturn)) Default_Handler()
{
    while(1);
}

void Reset_Handler() __attribute__((weak));
void NMI_Handler() __attribute__((weak, noreturn, alias("Default_Handler")));
void HardFault_Handler() __attribute__((weak, noreturn, alias("Default_Handler")));
void SVC_Handler() __attribute__((weak, noreturn, alias("Default_Handler")));
void PendSV_Handler() __attribute__((weak, noreturn, alias("Default_Handler")));
void SysTick_Handler() __attribute__((weak, noreturn, alias("Default_Handler")));

/* STM32F091 peripheral interrupts */
void WWDG_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void PVD_VDDIO2_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void RTC_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void FLASH_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void RCC_CRS_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void EXTI0_1_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void EXTI2_3_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void EXTI4_15_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TSC_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void DMA1_CH1_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void DMA1_CH2_3_DMA2_CH1_2_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void DMA1_CH4_5_6_7_DMA2_CH3_4_5_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void ADC_COMP_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM1_BRK_UP_TRG_COM_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM1_CC_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM2_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM3_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM6_DAC_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM7_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM14_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM15_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM16_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void TIM17_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void I2C1_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void I2C2_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void SPI1_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void SPI2_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void USART1_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void USART2_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void USART3_4_5_6_7_8_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void CEC_CAN_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));
void USB_IRQHandler() __attribute__((weak, noreturn, alias("Default_Handler")));

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

void (* const interrupt_vector_table[])() __attribute__((section(".isr_vector"))) = {
    (void (*)())_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    RESERVED,                                   /* Reserved */
    RESERVED,                                   /* Reserved */
    RESERVED,                                   /* Reserved */
    RESERVED,                                   /* Reserved */
    RESERVED,                                   /* Reserved */
    RESERVED,                                   /* Reserved */
    RESERVED,                                   /* Reserved */
    SVC_Handler,
    RESERVED,                                   /* Reserved */
    RESERVED,                                   /* Reserved */
    PendSV_Handler,
    SysTick_Handler,
    /* STM32F091 peripheral interrupts */
    WWDG_IRQHandler,                            /* 0 */
    PVD_VDDIO2_IRQHandler,                      /* 1 */
    RTC_IRQHandler,                             /* 2 */
    FLASH_IRQHandler,                           /* 3 */
    RCC_CRS_IRQHandler,                         /* 4 */
    EXTI0_1_IRQHandler,                         /* 5 */
    EXTI2_3_IRQHandler,                         /* 6 */
    EXTI4_15_IRQHandler,                        /* 7 */
    TSC_IRQHandler,                             /* 8 */
    DMA1_CH1_IRQHandler,                        /* 9 */
    DMA1_CH2_3_DMA2_CH1_2_IRQHandler,          /* 10 */
    DMA1_CH4_5_6_7_DMA2_CH3_4_5_IRQHandler,    /* 11 */
    ADC_COMP_IRQHandler,                        /* 12 */
    TIM1_BRK_UP_TRG_COM_IRQHandler,            /* 13 */
    TIM1_CC_IRQHandler,                         /* 14 */
    TIM2_IRQHandler,                            /* 15 */
    TIM3_IRQHandler,                            /* 16 */
    TIM6_DAC_IRQHandler,                        /* 17 */
    TIM7_IRQHandler,                            /* 18 */
    TIM14_IRQHandler,                           /* 19 */
    TIM15_IRQHandler,                           /* 20 */
    TIM16_IRQHandler,                           /* 21 */
    TIM17_IRQHandler,                           /* 22 */
    I2C1_IRQHandler,                            /* 23 */
    I2C2_IRQHandler,                            /* 24 */
    SPI1_IRQHandler,                            /* 25 */
    SPI2_IRQHandler,                            /* 26 */
    USART1_IRQHandler,                          /* 27 */
    USART2_IRQHandler,                          /* 28 */
    USART3_4_5_6_7_8_IRQHandler,               /* 29 */
    CEC_CAN_IRQHandler,                         /* 30 */
    USB_IRQHandler,                             /* 31 */
};

void __attribute__((naked)) Reset_Handler()
{
    __asm__("ldr r0, =_estack\n\t"
            "mov sp, r0");

    /* Copy data section from flash to RAM */
    memcpy(_sdata, _sidata, (uint8_t *)_edata - (uint8_t *)_sdata);

    /* Zero out bss */
    memset(_sbss, 0, (uint8_t *)_ebss - (uint8_t *)_sbss);

    /* Set Interrupt Vector Table Offset */
    uint32_t *vtor = (uint32_t *)0xE000ED08;
    *vtor = (uint32_t)interrupt_vector_table;

    main();
}
