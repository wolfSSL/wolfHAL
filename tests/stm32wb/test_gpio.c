#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/gpio/stm32wb_gpio.h>
#include <wolfHAL/bitops.h>
#include "stm32wb55xx_nucleo.h"
#include "../test.h"

/*
 * GPIO register offsets (GPIOB port base = GPIO base + 0x400)
 * LED is on PB5.
 */
#define GPIOB_BASE_OFFSET 0x400
#define GPIOx_MODE_REG    0x00
#define GPIOx_ODR_REG     0x14

static void test_gpio_mode_register(void)
{
    /* PB5 should be configured as output (mode = 0x01) in bits [11:10] */
    size_t portBase = whal_dev_gpio.regmap.base + GPIOB_BASE_OFFSET;
    size_t mask = WHAL_MASK_RANGE(11, 10);
    size_t val = 0;

    whal_Reg_Get(portBase, GPIOx_MODE_REG, mask, &val);
    WHAL_ASSERT_EQ(val, WHAL_STM32WB_GPIO_MODE_OUT);
}

static void test_gpio_set_high(void)
{
    WHAL_ASSERT_EQ(WHAL_GPIO_SET(gpio, LED_PIN, 1), WHAL_SUCCESS);

    /* Readback ODR bit 5 */
    size_t portBase = whal_dev_gpio.regmap.base + GPIOB_BASE_OFFSET;
    size_t val = 0;
    whal_Reg_Get(portBase, GPIOx_ODR_REG, WHAL_MASK(5), &val);
    WHAL_ASSERT_EQ(val, 1);
}

static void test_gpio_set_low(void)
{
    WHAL_ASSERT_EQ(WHAL_GPIO_SET(gpio, LED_PIN, 0), WHAL_SUCCESS);

    size_t portBase = whal_dev_gpio.regmap.base + GPIOB_BASE_OFFSET;
    size_t val = 0;
    whal_Reg_Get(portBase, GPIOx_ODR_REG, WHAL_MASK(5), &val);
    WHAL_ASSERT_EQ(val, 0);
}

void test_gpio(void)
{
    WHAL_TEST_SUITE_START("gpio");
    WHAL_TEST(test_gpio_mode_register);
    WHAL_TEST(test_gpio_set_high);
    WHAL_TEST(test_gpio_set_low);
    WHAL_TEST_SUITE_END();
}
