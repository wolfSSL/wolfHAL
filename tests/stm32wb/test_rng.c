#include <stdint.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/clock/stm32wb_rcc.h>
#include <wolfHAL/rng/stm32wb_rng.h>
#include <wolfHAL/bitops.h>
#include "stm32wb55xx_nucleo.h"
#include "../test.h"

static void test_rng_init_deinit(void)
{
    WHAL_ASSERT_EQ(WHAL_RNG_INIT(rng), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_RNG_DEINIT(rng), WHAL_SUCCESS);
}

static void test_rng_generate_nonzero(void)
{
    uint8_t buf[16] = {0};
    int allZero = 1;

    whal_Stm32wbRcc_Ext_EnableHsi48(&whal_dev_clock, 1);
    WHAL_ASSERT_EQ(WHAL_RNG_INIT(rng), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_RNG_GENERATE(rng, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_RNG_DEINIT(rng), WHAL_SUCCESS);
    whal_Stm32wbRcc_Ext_EnableHsi48(&whal_dev_clock, 0);

    for (size_t i = 0; i < sizeof(buf); i++) {
        if (buf[i] != 0) {
            allZero = 0;
            break;
        }
    }

    /* 16 zero bytes from a TRNG is astronomically unlikely */
    WHAL_ASSERT_EQ(allZero, 0);
}

static void test_rng_generate_unique(void)
{
    uint8_t buf1[16] = {0};
    uint8_t buf2[16] = {0};
    int same = 1;

    whal_Stm32wbRcc_Ext_EnableHsi48(&whal_dev_clock, 1);
    WHAL_ASSERT_EQ(WHAL_RNG_INIT(rng), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_RNG_GENERATE(rng, buf1, sizeof(buf1)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_RNG_GENERATE(rng, buf2, sizeof(buf2)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_RNG_DEINIT(rng), WHAL_SUCCESS);
    whal_Stm32wbRcc_Ext_EnableHsi48(&whal_dev_clock, 0);

    for (size_t i = 0; i < sizeof(buf1); i++) {
        if (buf1[i] != buf2[i]) {
            same = 0;
            break;
        }
    }

    /* Two consecutive 16-byte outputs should differ */
    WHAL_ASSERT_EQ(same, 0);
}

void test_rng(void)
{
    WHAL_TEST_SUITE_START("rng");
    WHAL_TEST(test_rng_init_deinit);
    WHAL_TEST(test_rng_generate_nonzero);
    WHAL_TEST(test_rng_generate_unique);
    WHAL_TEST_SUITE_END();
}
