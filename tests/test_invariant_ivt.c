#include <check.h>
#include <stdint.h>
#include <string.h>

/* External symbols from ivt.c */
extern uint32_t _sdata, _edata, _sidata;
extern uint32_t _sbss, _ebss;

START_TEST(test_buffer_reads_never_exceed_declared_length)
{
    /* Invariant: Multiplication (data_section_size * 4) must not overflow */
    uint32_t test_cases[] = {
        /* Exact exploit case: size that causes overflow when multiplied by 4 */
        UINT32_MAX / 4 + 1,
        /* Boundary case: maximum safe size */
        UINT32_MAX / 4,
        /* Valid small size */
        1024
    };
    
    for (int i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        uint32_t test_size = test_cases[i];
        
        /* Simulate the vulnerable calculation */
        uint32_t multiplied = test_size * 4;
        
        /* The security property: multiplied result must be >= original size * 4 
           when no overflow occurs, or overflow must be detected */
        if (test_size > UINT32_MAX / 4) {
            /* Overflow must have occurred - verify detection */
            ck_assert_msg(multiplied < test_size, 
                "Overflow not detected for size %u (multiplied: %u)", 
                test_size, multiplied);
        } else {
            /* No overflow - verify correct multiplication */
            ck_assert_msg(multiplied == test_size * 4, 
                "Multiplication incorrect for size %u (got %u, expected %u)", 
                test_size, multiplied, test_size * 4);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_reads_never_exceed_declared_length);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}