#include <stdint.h>
#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/timer/timer.h>
#include <wolfHAL/rng/rng.h>
#include "../test.h"

/*
 * Mock drivers for compile-time dispatch testing.
 * Each function returns SUCCESS and optionally sets output values.
 * Functions are non-static because DEV_DECLARE inline wrappers call them.
 */

#define MOCK_REGMAP { .base = 0, .size = 0 }

/* --- Clock mock --- */
whal_Error WHAL_DRV_FN(MockClock, init)(whal_Clock *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockClock, deinit)(whal_Clock *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockClock, enable)(whal_Clock *d, const void *c) { (void)d; (void)c; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockClock, disable)(whal_Clock *d, const void *c) { (void)d; (void)c; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockClock, getrate)(whal_Clock *d, size_t *r) { (void)d; *r = 64000000; return WHAL_SUCCESS; }

WHAL_CLOCK_DEV_DECLARE(mockClock, MockClock)
WHAL_CLOCK_DEV_DEFINE(mockClock, MockClock, MOCK_REGMAP, NULL);

/* --- GPIO mock --- */
whal_Error WHAL_DRV_FN(MockGpio, init)(whal_Gpio *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockGpio, deinit)(whal_Gpio *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockGpio, get)(whal_Gpio *d, size_t p, size_t *v) { (void)d; (void)p; *v = 1; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockGpio, set)(whal_Gpio *d, size_t p, size_t v) { (void)d; (void)p; (void)v; return WHAL_SUCCESS; }

WHAL_GPIO_DEV_DECLARE(mockGpio, MockGpio)
WHAL_GPIO_DEV_DEFINE(mockGpio, MockGpio, MOCK_REGMAP, NULL);

/* --- UART mock --- */
whal_Error WHAL_DRV_FN(MockUart, init)(whal_Uart *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockUart, deinit)(whal_Uart *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockUart, send)(whal_Uart *d, const uint8_t *data, size_t sz) { (void)d; (void)data; (void)sz; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockUart, recv)(whal_Uart *d, uint8_t *data, size_t sz) { (void)d; (void)data; (void)sz; return WHAL_SUCCESS; }

WHAL_UART_DEV_DECLARE(mockUart, MockUart)
WHAL_UART_DEV_DEFINE(mockUart, MockUart, MOCK_REGMAP, NULL);

/* --- Flash mock --- */
whal_Error WHAL_DRV_FN(MockFlash, init)(whal_Flash *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockFlash, deinit)(whal_Flash *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockFlash, lock)(whal_Flash *d, size_t a, size_t l) { (void)d; (void)a; (void)l; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockFlash, unlock)(whal_Flash *d, size_t a, size_t l) { (void)d; (void)a; (void)l; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockFlash, read)(whal_Flash *d, size_t a, uint8_t *data, size_t sz) { (void)d; (void)a; (void)data; (void)sz; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockFlash, write)(whal_Flash *d, size_t a, const uint8_t *data, size_t sz) { (void)d; (void)a; (void)data; (void)sz; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockFlash, erase)(whal_Flash *d, size_t a, size_t sz) { (void)d; (void)a; (void)sz; return WHAL_SUCCESS; }

WHAL_FLASH_DEV_DECLARE(mockFlash, MockFlash)
WHAL_FLASH_DEV_DEFINE(mockFlash, MockFlash, MOCK_REGMAP, NULL);

/* --- Timer mock --- */
whal_Error WHAL_DRV_FN(MockTimer, init)(whal_Timer *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockTimer, deinit)(whal_Timer *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockTimer, start)(whal_Timer *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockTimer, stop)(whal_Timer *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockTimer, reset)(whal_Timer *d) { (void)d; return WHAL_SUCCESS; }

WHAL_TIMER_DEV_DECLARE(mockTimer, MockTimer)
WHAL_TIMER_DEV_DEFINE(mockTimer, MockTimer, MOCK_REGMAP, NULL);

/* --- RNG mock --- */
whal_Error WHAL_DRV_FN(MockRng, init)(whal_Rng *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockRng, deinit)(whal_Rng *d) { (void)d; return WHAL_SUCCESS; }
whal_Error WHAL_DRV_FN(MockRng, generate)(whal_Rng *d, uint8_t *data, size_t sz) { (void)d; (void)data; (void)sz; return WHAL_SUCCESS; }

WHAL_RNG_DEV_DECLARE(mockRng, MockRng)
WHAL_RNG_DEV_DEFINE(mockRng, MockRng, MOCK_REGMAP, NULL);

/* --- Clock dispatch tests --- */

static void test_clock_valid_dispatch(void)
{
    WHAL_ASSERT_EQ(WHAL_CLOCK_INIT(mockClock), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_CLOCK_DEINIT(mockClock), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_CLOCK_ENABLE(mockClock, NULL), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_CLOCK_DISABLE(mockClock, NULL), WHAL_SUCCESS);
    size_t rate;
    WHAL_ASSERT_EQ(WHAL_CLOCK_GETRATE(mockClock, &rate), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(rate, 64000000);
}

/* --- GPIO dispatch tests --- */

static void test_gpio_valid_dispatch(void)
{
    WHAL_ASSERT_EQ(WHAL_GPIO_INIT(mockGpio), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_GPIO_SET(mockGpio, 0, 1), WHAL_SUCCESS);
    size_t val;
    WHAL_ASSERT_EQ(WHAL_GPIO_GET(mockGpio, 0, &val), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val, 1);
}

/* --- UART dispatch tests --- */

static void test_uart_valid_dispatch(void)
{
    WHAL_ASSERT_EQ(WHAL_UART_INIT(mockUart), WHAL_SUCCESS);
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(WHAL_UART_SEND(mockUart, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_UART_RECV(mockUart, buf, sizeof(buf)), WHAL_SUCCESS);
}

/* --- Flash dispatch tests --- */

static void test_flash_valid_dispatch(void)
{
    WHAL_ASSERT_EQ(WHAL_FLASH_INIT(mockFlash), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_FLASH_LOCK(mockFlash, 0, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_FLASH_UNLOCK(mockFlash, 0, 0), WHAL_SUCCESS);
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(WHAL_FLASH_READ(mockFlash, 0, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_FLASH_WRITE(mockFlash, 0, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_FLASH_ERASE(mockFlash, 0, sizeof(buf)), WHAL_SUCCESS);
}

/* --- Timer dispatch tests --- */

static void test_timer_valid_dispatch(void)
{
    WHAL_ASSERT_EQ(WHAL_TIMER_INIT(mockTimer), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_TIMER_START(mockTimer), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_TIMER_STOP(mockTimer), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_TIMER_RESET(mockTimer), WHAL_SUCCESS);
}

/* --- RNG dispatch tests --- */

static void test_rng_valid_dispatch(void)
{
    WHAL_ASSERT_EQ(WHAL_RNG_INIT(mockRng), WHAL_SUCCESS);
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(WHAL_RNG_GENERATE(mockRng, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(WHAL_RNG_DEINIT(mockRng), WHAL_SUCCESS);
}

/* --- Runtime dispatch tests (only when WHAL_RUNTIME_POLYMORPHISM is set) --- */

#ifdef WHAL_RUNTIME_POLYMORPHISM

static void test_clock_runtime_dispatch(void)
{
    WHAL_ASSERT_EQ(whal_Clock_Init(&whal_dev_mockClock), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Clock_Deinit(&whal_dev_mockClock), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Clock_Enable(&whal_dev_mockClock, NULL), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Clock_Disable(&whal_dev_mockClock, NULL), WHAL_SUCCESS);
    size_t rate;
    WHAL_ASSERT_EQ(whal_Clock_Getrate(&whal_dev_mockClock, &rate), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(rate, 64000000);
}

static void test_gpio_runtime_dispatch(void)
{
    WHAL_ASSERT_EQ(whal_Gpio_Init(&whal_dev_mockGpio), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Gpio_Set(&whal_dev_mockGpio, 0, 1), WHAL_SUCCESS);
    size_t val;
    WHAL_ASSERT_EQ(whal_Gpio_Get(&whal_dev_mockGpio, 0, &val), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(val, 1);
}

static void test_uart_runtime_dispatch(void)
{
    WHAL_ASSERT_EQ(whal_Uart_Init(&whal_dev_mockUart), WHAL_SUCCESS);
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(whal_Uart_Send(&whal_dev_mockUart, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Uart_Recv(&whal_dev_mockUart, buf, sizeof(buf)), WHAL_SUCCESS);
}

static void test_flash_runtime_dispatch(void)
{
    WHAL_ASSERT_EQ(whal_Flash_Init(&whal_dev_mockFlash), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Lock(&whal_dev_mockFlash, 0, 0), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Unlock(&whal_dev_mockFlash, 0, 0), WHAL_SUCCESS);
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(whal_Flash_Read(&whal_dev_mockFlash, 0, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Write(&whal_dev_mockFlash, 0, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Flash_Erase(&whal_dev_mockFlash, 0, sizeof(buf)), WHAL_SUCCESS);
}

static void test_timer_runtime_dispatch(void)
{
    WHAL_ASSERT_EQ(whal_Timer_Init(&whal_dev_mockTimer), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Timer_Start(&whal_dev_mockTimer), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Timer_Stop(&whal_dev_mockTimer), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Timer_Reset(&whal_dev_mockTimer), WHAL_SUCCESS);
}

static void test_rng_runtime_dispatch(void)
{
    WHAL_ASSERT_EQ(whal_Rng_Init(&whal_dev_mockRng), WHAL_SUCCESS);
    uint8_t buf[4] = {0};
    WHAL_ASSERT_EQ(whal_Rng_Generate(&whal_dev_mockRng, buf, sizeof(buf)), WHAL_SUCCESS);
    WHAL_ASSERT_EQ(whal_Rng_Deinit(&whal_dev_mockRng), WHAL_SUCCESS);
}

#endif /* WHAL_RUNTIME_POLYMORPHISM */

void test_dispatch(void)
{
    WHAL_TEST_SUITE_START("dispatch");
    WHAL_TEST(test_clock_valid_dispatch);
    WHAL_TEST(test_gpio_valid_dispatch);
    WHAL_TEST(test_uart_valid_dispatch);
    WHAL_TEST(test_flash_valid_dispatch);
    WHAL_TEST(test_timer_valid_dispatch);
    WHAL_TEST(test_rng_valid_dispatch);
#ifdef WHAL_RUNTIME_POLYMORPHISM
    WHAL_TEST(test_clock_runtime_dispatch);
    WHAL_TEST(test_gpio_runtime_dispatch);
    WHAL_TEST(test_uart_runtime_dispatch);
    WHAL_TEST(test_flash_runtime_dispatch);
    WHAL_TEST(test_timer_runtime_dispatch);
    WHAL_TEST(test_rng_runtime_dispatch);
#endif
    WHAL_TEST_SUITE_END();
}
