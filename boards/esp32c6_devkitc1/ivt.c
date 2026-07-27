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

/*
 * ESP32-C6 High-Performance CPU (RV32IMAC) startup and machine trap vector.
 *
 * Unlike the Cortex-M boards in this repo, RISC-V has no hardware-loaded vector
 * table: after reset the core simply begins executing, so the startup code must
 * establish its own stack, global pointer, and trap vector.
 *
 * Boot order under Direct Boot:
 *   ROM  -> _start (0x4200_0008)  -> Reset_Handler -> main()
 */

#include <stdint.h>
#include <stddef.h>

/* Section boundaries provided by linker.ld. */
extern uint32_t _sidata[];   /* .data initial image, in flash (LMA)   */
extern uint32_t _sdata[];    /* .data start in SRAM (VMA)             */
extern uint32_t _edata[];    /* .data end in SRAM                    */
extern uint32_t _sbss[];     /* .bss start in SRAM                   */
extern uint32_t _ebss[];     /* .bss end in SRAM                     */
extern uint32_t _estack[];   /* top of the stack                    */

extern void main(void);
void Reset_Handler(void);
void _default_handler(void);
extern void _vector_table(void);

/*
 * Machine trap vector table.
 *
 * The ESP32-C6 mtvec CSR is vectored-only with a read-only MODE field hardwired
 * to 0x1 (TRM Reg 1.9). In vectored mode the core computes the trap target as
 * (mtvec + 4 * cause): synchronous exceptions land at slot 0, interrupt cause n
 * lands at slot n. Every slot must therefore be exactly one 4-byte jump
 * instruction. `.option norvc` forbids the assembler from emitting a compressed
 * 2-byte c.j (RV32IMAC has the C extension), which would corrupt the 4-byte
 * stride. `aligned(256)` matches the 256-byte base-alignment mtvec requires.
 *
 * Every slot defaults to _default_handler. To service a specific interrupt,
 * replace its `j _default_handler` with a jump to a dedicated handler.
 */
__attribute__((naked, aligned(256), section(".vectors")))
void _vector_table(void)
{
    __asm__ volatile(
        ".option push       \n"
        ".option norvc      \n"
        "j _default_handler \n"   /*  0  exception entry                     */
        "j _default_handler \n"   /*  1                                      */
        "j _default_handler \n"   /*  2                                      */
        "j _default_handler \n"   /*  3                                      */
        "j _default_handler \n"   /*  4                                      */
        "j _default_handler \n"   /*  5                                      */
        "j _default_handler \n"   /*  6                                      */
        "j _default_handler \n"   /*  7  (CLINT: machine software interrupt) */
        "j _default_handler \n"   /*  8                                      */
        "j _default_handler \n"   /*  9                                      */
        "j _default_handler \n"   /* 10                                      */
        "j _default_handler \n"   /* 11  (CLINT: machine external interrupt) */
        "j _default_handler \n"   /* 12                                      */
        "j _default_handler \n"   /* 13                                      */
        "j _default_handler \n"   /* 14                                      */
        "j _default_handler \n"   /* 15                                      */
        "j _default_handler \n"   /* 16                                      */
        "j _default_handler \n"   /* 17                                      */
        "j _default_handler \n"   /* 18                                      */
        "j _default_handler \n"   /* 19                                      */
        "j _default_handler \n"   /* 20                                      */
        "j _default_handler \n"   /* 21                                      */
        "j _default_handler \n"   /* 22                                      */
        "j _default_handler \n"   /* 23                                      */
        "j _default_handler \n"   /* 24                                      */
        "j _default_handler \n"   /* 25                                      */
        "j _default_handler \n"   /* 26                                      */
        "j _default_handler \n"   /* 27                                      */
        "j _default_handler \n"   /* 28                                      */
        "j _default_handler \n"   /* 29                                      */
        "j _default_handler \n"   /* 30                                      */
        "j _default_handler \n"   /* 31                                      */
        ".option pop        \n"
    );
}

/* Catch-all trap handler: park the core so a stray trap is observable on a
 * debugger rather than silently corrupting state. */
void __attribute__((noreturn)) _default_handler(void)
{
    while (1) {
    }
}

/*
 * Reset entry. Runs from flash (XIP) the moment the ROM hands over control.
 * `naked` keeps the compiler from emitting a prologue that would touch a stack
 * that does not yet exist; everything here is hand-written assembly that sets
 * up the C environment, then calls into Reset_Handler for the rest.
 *
 *   - gp (global pointer) is loaded with relaxation disabled so the assembler
 *     does not "optimize" the load into a nonsensical `mv gp, gp`.
 *   - sp is set to the top of HP SRAM.
 */
__attribute__((naked, section(".text.entry")))
void _start(void)
{
    __asm__ volatile(
        ".option push          \n"
        ".option norelax       \n"
        "la gp, __global_pointer$\n"
        ".option pop           \n"
        "la sp, _estack        \n"
        "call Reset_Handler    \n"
        /* Reset_Handler should call main() and not return. If it ever does,
         * park the core instead of running off into undefined memory. */
        "1: j 1b               \n"
    );
}

/*
 * C-runtime bring-up. On entry sp and gp are valid (set by _start), the core
 * runs from flash, and interrupts are globally disabled (mstatus.MIE resets to
 * 0). Prepare the C environment, point mtvec at the trap table, then hand off to
 * main().
 *
 * Global interrupts stay disabled: this build wires up no ISRs, so any stray
 * trap parks in _default_handler rather than vectoring into empty slots. Enable
 * them (and configure the interrupt controller) once a real ISR exists.
 *
 * The .data and .bss bounds are word-aligned by linker.ld, so both loops move a
 * word at a time.
 */
void Reset_Handler(void)
{
    uint32_t *src = _sidata;
    uint32_t *dst = _sdata;

    /* Copy initialized data from its flash load address into RAM. */
    while (dst < _edata)
        *dst++ = *src++;

    /* Zero-initialize .bss. */
    for (dst = _sbss; dst < _ebss; dst++)
        *dst = 0;

    /* Point the machine trap vector at our table (MODE is hardwired vectored). */
    __asm__ volatile("csrw mtvec, %0" :: "r"(&_vector_table));

    main();
}
