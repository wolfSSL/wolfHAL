#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/clock/pic32cz_clock.h>
#include <wolfHAL/bitops.h>
#include "pic32cz_curiosity_ultra.h"
#include "../test.h"

/*
 * GCLK peripheral channel control register layout:
 *   Offset from clock base: 0x10000 + 0x80 + (channel * 4)
 *   GEN field: bits [3:0]
 *   CHEN bit: bit 6
 *
 * MCLK clock mask register layout:
 *   Offset from clock base: 0x12000 + 0x3C + (inst * 4)
 */
#define GCLK_PCHCTRL_OFFSET(ch) (0x10000 + 0x80 + ((ch) * 4))
#define GCLK_PCHCTRL_CHEN       WHAL_MASK(6)

#define MCLK_CLKMSK_OFFSET(inst) (0x12000 + 0x3C + ((inst) * 4))

static void test_clock_enable_disable(void)
{
    /* Use a test clock descriptor for SERCOM4 (same as UART clock) */
    whal_Pic32czClock_Clk testClk = {
        .gclkPeriphChannel = 25,
        .gclkPeriphSrc = 0,
        .mclkEnableInst = 1,
        .mclkEnableMask = WHAL_MASK(3),
    };

    size_t val = 0;

    WHAL_ASSERT_EQ(WHAL_CLOCK_ENABLE(clock, &testClk), WHAL_SUCCESS);

    /* Readback: GCLK PCHCTRL channel 25 CHEN bit should be set */
    whal_Reg_Get(whal_dev_clock.regmap.base, GCLK_PCHCTRL_OFFSET(25),
                 GCLK_PCHCTRL_CHEN, &val);
    WHAL_ASSERT_EQ(val, 1);

    /* Readback: MCLK CLKMSK1 bit 3 should be set */
    whal_Reg_Get(whal_dev_clock.regmap.base, MCLK_CLKMSK_OFFSET(1),
                 WHAL_MASK(3), &val);
    WHAL_ASSERT_EQ(val, 1);

    WHAL_ASSERT_EQ(WHAL_CLOCK_DISABLE(clock, &testClk), WHAL_SUCCESS);

    /* After disable: CHEN should be cleared */
    whal_Reg_Get(whal_dev_clock.regmap.base, GCLK_PCHCTRL_OFFSET(25),
                 GCLK_PCHCTRL_CHEN, &val);
    WHAL_ASSERT_EQ(val, 0);

    /* Re-enable so UART still works for remaining tests */
    WHAL_ASSERT_EQ(WHAL_CLOCK_ENABLE(clock, &testClk), WHAL_SUCCESS);
}

void test_clock(void)
{
    WHAL_TEST_SUITE_START("clock");
    WHAL_TEST(test_clock_enable_disable);
    WHAL_TEST_SUITE_END();
}
