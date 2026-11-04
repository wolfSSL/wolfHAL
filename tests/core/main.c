/* main.c
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

#include <stdio.h>
#include "../test.h"

int g_whalTestPassed;
int g_whalTestFailed;
int g_whalTestSkipped;
int g_whalTestCurFailed;
int g_whalTestCurSkipped;

void whal_Test_Puts(const char *s)
{
    fputs(s, stdout);
}

void whal_Test_Bitops(void);
void whal_Test_Dispatch(void);
void whal_Test_Endian(void);
void whal_Test_Reg(void);
void whal_Test_Pl011_Uart(void);
#ifndef WHAL_CFG_NO_TIMEOUT
void whal_Test_Timeout(void);
#endif

int main(void)
{
    g_whalTestPassed = 0;
    g_whalTestFailed = 0;
    g_whalTestSkipped = 0;

    whal_Test_Bitops();
    whal_Test_Dispatch();
    whal_Test_Endian();
    whal_Test_Reg();
    whal_Test_Pl011_Uart();
#ifndef WHAL_CFG_NO_TIMEOUT
    whal_Test_Timeout();
#endif

    WHAL_TEST_SUMMARY();

    return g_whalTestFailed ? 1 : 0;
}
