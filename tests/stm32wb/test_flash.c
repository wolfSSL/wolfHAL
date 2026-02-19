#include <stdint.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/flash/stm32wb_flash.h>
#include <wolfHAL/bitops.h>
#include "stm32wb55xx_nucleo.h"
#include "../test.h"

/* Flash CR register offset and LOCK bit */
#define FLASH_CR_REG  0x14
#define FLASH_CR_LOCK WHAL_MASK(31)

/* Test address in the data flash area (OTP/data, not code flash) */
#define TEST_FLASH_ADDR 0x08080000
#define TEST_FLASH_SIZE 0x1000

static void test_flash_write_read(void)
{
    uint8_t pattern[] = "wolfHAL TEST";
    uint8_t readback[sizeof(pattern)] = {0};

    WHAL_ASSERT_EQ(WHAL_FLASH_UNLOCK(flash, 0, 0), WHAL_SUCCESS);

    WHAL_ASSERT_EQ(WHAL_FLASH_ERASE(flash, TEST_FLASH_ADDR, TEST_FLASH_SIZE),
              WHAL_SUCCESS);

    whal_Error err;
    do {
        err = WHAL_FLASH_WRITE(flash, TEST_FLASH_ADDR, pattern,
                               sizeof(pattern));
    } while (err == WHAL_ENOTREADY);
    WHAL_ASSERT_EQ(err, WHAL_SUCCESS);

    WHAL_ASSERT_EQ(WHAL_FLASH_READ(flash, TEST_FLASH_ADDR, readback,
                              sizeof(readback)),
              WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(pattern, readback, sizeof(pattern));

    WHAL_ASSERT_EQ(WHAL_FLASH_LOCK(flash, 0, 0), WHAL_SUCCESS);
}

static void test_flash_lock_readback(void)
{
    /* After locking, the CR.LOCK bit should be set */
    WHAL_ASSERT_EQ(WHAL_FLASH_LOCK(flash, 0, 0), WHAL_SUCCESS);

    size_t val = 0;
    whal_Reg_Get(whal_dev_flash.regmap.base, FLASH_CR_REG, FLASH_CR_LOCK, &val);
    WHAL_ASSERT_EQ(val, 1);
}

void test_flash(void)
{
    WHAL_TEST_SUITE_START("flash");
    WHAL_TEST(test_flash_write_read);
    WHAL_TEST(test_flash_lock_readback);
    WHAL_TEST_SUITE_END();
}
