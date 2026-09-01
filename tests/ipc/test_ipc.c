/* test_ipc.c
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

#include <wolfHAL/wolfHAL.h>
#include "wolfHAL_board.h"
#include "test.h"

static void Test_Ipc_InitDeinit(void)
{
    WHAL_ASSERT_EQ(whal_Ipc_Init(BOARD_IPC_DEV), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Ipc_Deinit(BOARD_IPC_DEV), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Ipc_Init(BOARD_IPC_DEV), WHAL_SUCCESS);
}

static void Test_Ipc_SendNull(void)
{
    WHAL_ASSERT_EQ(whal_Ipc_Send(BOARD_IPC_DEV, NULL, 0), WHAL_EINVAL);
}

static void Test_Ipc_RecvNull(void)
{
    WHAL_ASSERT_EQ(whal_Ipc_Recv(BOARD_IPC_DEV, NULL, 0), WHAL_EINVAL);
}

void whal_Test_Ipc(void)
{
    WHAL_TEST_SUITE_START("ipc");
    WHAL_TEST(Test_Ipc_InitDeinit);
    WHAL_TEST(Test_Ipc_SendNull);
    WHAL_TEST(Test_Ipc_RecvNull);
    WHAL_TEST_SUITE_END();
}
