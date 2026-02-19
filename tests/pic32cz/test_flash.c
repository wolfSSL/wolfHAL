#include <stdint.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/flash/pic32cz_flash.h>
#include <wolfHAL/bitops.h>
#include "pic32cz_curiosity_ultra.h"
#include "../test.h"

/* Test address in PFM (Program Flash Memory) data area */
#define TEST_FLASH_ADDR 0x0C000000
#define TEST_FLASH_SIZE 0x1000

static void test_flash_write_read(void)
{
    uint8_t pattern[] = "wolfHAL";  /* 8 bytes, double-word aligned */
    uint8_t readback[sizeof(pattern)] = {0};

    WHAL_ASSERT_EQ(WHAL_FLASH_ERASE(flash, TEST_FLASH_ADDR,
                                     TEST_FLASH_SIZE),
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
}

static void test_flash_erase_blank(void)
{
    uint8_t readback[8] = {0};
    uint8_t erased[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    WHAL_ASSERT_EQ(WHAL_FLASH_ERASE(flash, TEST_FLASH_ADDR,
                                     TEST_FLASH_SIZE),
                   WHAL_SUCCESS);

    WHAL_ASSERT_EQ(WHAL_FLASH_READ(flash, TEST_FLASH_ADDR, readback,
                                    sizeof(readback)),
                   WHAL_SUCCESS);

    WHAL_ASSERT_MEM_EQ(readback, erased, sizeof(erased));
}

void test_flash(void)
{
    WHAL_TEST_SUITE_START("flash");
    WHAL_TEST(test_flash_erase_blank);
    WHAL_TEST(test_flash_write_read);
    WHAL_TEST_SUITE_END();
}
