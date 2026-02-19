#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/clock/stm32wb_rcc.h>
#include <wolfHAL/bitops.h>
#include "stm32wb55xx_nucleo.h"
#include "../test.h"

static void test_clock_getrate(void)
{
    size_t rate = 0;
    WHAL_ASSERT_EQ(WHAL_CLOCK_GETRATE(clock, &rate), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(rate, 64000000);
}

static void test_clock_enable_disable(void)
{
    whal_Stm32wbRcc_Clk testClk = { WHAL_STM32WB55_GPIOA_CLOCK };

    WHAL_ASSERT_EQ(WHAL_CLOCK_ENABLE(clock, &testClk), WHAL_SUCCESS);

    /* Readback: GPIOA enable bit should be set in AHB2ENR (offset 0x4C, bit 0) */
    size_t val = 0;
    whal_Reg_Get(whal_dev_clock.regmap.base, 0x4C, (1 << 0), &val);
    WHAL_ASSERT_EQ(val, 1);

    WHAL_ASSERT_EQ(WHAL_CLOCK_DISABLE(clock, &testClk), WHAL_SUCCESS);

    whal_Reg_Get(whal_dev_clock.regmap.base, 0x4C, (1 << 0), &val);
    WHAL_ASSERT_EQ(val, 0);
}

void test_clock(void)
{
    WHAL_TEST_SUITE_START("clock");
    WHAL_TEST(test_clock_getrate);
    WHAL_TEST(test_clock_enable_disable);
    WHAL_TEST_SUITE_END();
}
