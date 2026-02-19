#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/gpio/pic32cz_gpio.h>
#include <wolfHAL/bitops.h>
#include "pic32cz_curiosity_ultra.h"
#include "../test.h"

/* Pin indices matching board config pin table */
enum {
    LED_PIN,
    UART_TX_PIN,
    UART_RX_PIN,
};

/*
 * PIC32CZ PORT register offsets.
 * Each port group is 0x80 bytes. LED is on port B (index 1), pin 21.
 */
#define PORT_DIR_REG(port) (0x00 + ((port) * 0x80))
#define PORT_OUT_REG(port) (0x10 + ((port) * 0x80))

/* LED pin: port B, pin 21 */
#define LED_PORT 1
#define LED_HW_PIN 21

static void test_gpio_dir_register(void)
{
    /* PB21 should be configured as output (bit 21 set in DIR register) */
    size_t val = 0;
    whal_Reg_Get(whal_dev_gpio.regmap.base, PORT_DIR_REG(LED_PORT),
                 WHAL_MASK(LED_HW_PIN), &val);
    WHAL_ASSERT_EQ(val, 1);
}

static void test_gpio_set_high(void)
{
    WHAL_ASSERT_EQ(WHAL_GPIO_SET(gpio, LED_PIN, 1), WHAL_SUCCESS);

    /* Readback OUT register bit 21 */
    size_t val = 0;
    whal_Reg_Get(whal_dev_gpio.regmap.base, PORT_OUT_REG(LED_PORT),
                 WHAL_MASK(LED_HW_PIN), &val);
    WHAL_ASSERT_EQ(val, 1);
}

static void test_gpio_set_low(void)
{
    WHAL_ASSERT_EQ(WHAL_GPIO_SET(gpio, LED_PIN, 0), WHAL_SUCCESS);

    size_t val = 0;
    whal_Reg_Get(whal_dev_gpio.regmap.base, PORT_OUT_REG(LED_PORT),
                 WHAL_MASK(LED_HW_PIN), &val);
    WHAL_ASSERT_EQ(val, 0);
}

void test_gpio(void)
{
    WHAL_TEST_SUITE_START("gpio");
    WHAL_TEST(test_gpio_dir_register);
    WHAL_TEST(test_gpio_set_high);
    WHAL_TEST(test_gpio_set_low);
    WHAL_TEST_SUITE_END();
}
