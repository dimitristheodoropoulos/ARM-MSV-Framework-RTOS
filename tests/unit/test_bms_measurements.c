#include <stdio.h>

#include "bms_measurements.h"

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_TRUE(condition)                                      \
    do                                                              \
    {                                                               \
        tests_run++;                                                \
        if (!(condition))                                           \
        {                                                           \
            fprintf(stderr,                                          \
                    "[FAIL] %s:%d: %s\n",                           \
                    __FILE__,                                       \
                    __LINE__,                                       \
                    #condition);                                    \
            return 1;                                               \
        }                                                           \
        tests_passed++;                                             \
    } while (0)

#define ASSERT_EQ(expected, actual)                                 \
    do                                                              \
    {                                                               \
        tests_run++;                                                \
        if ((expected) != (actual))                                 \
        {                                                           \
            fprintf(stderr,                                          \
                    "[FAIL] %s:%d: expected %d, got %d\n",          \
                    __FILE__,                                       \
                    __LINE__,                                       \
                    (expected),                                     \
                    (actual));                                      \
            return 1;                                               \
        }                                                           \
        tests_passed++;                                             \
    } while (0)

static int test_init_sets_measurements_unavailable(void)
{
    bms_measurements_t measurements;

    measurements.voltage.value = 100.0f;
    measurements.voltage.status = BMS_MEAS_VALID;

    measurements.current.value = 10.0f;
    measurements.current.status = BMS_MEAS_VALID;

    measurements.temperature.value = 25.0f;
    measurements.temperature.status = BMS_MEAS_VALID;

    bms_measurements_init(&measurements);

    ASSERT_TRUE(measurements.voltage.value == 0.0f);
    ASSERT_EQ(BMS_MEAS_NOT_AVAILABLE,
              measurements.voltage.status);

    ASSERT_TRUE(measurements.current.value == 0.0f);
    ASSERT_EQ(BMS_MEAS_NOT_AVAILABLE,
              measurements.current.status);

    ASSERT_TRUE(measurements.temperature.value == 0.0f);
    ASSERT_EQ(BMS_MEAS_NOT_AVAILABLE,
              measurements.temperature.status);

    return 0;
}

static int test_init_null_pointer(void)
{
    bms_measurements_init(NULL);

    /*
     * Reaching this point confirms that NULL input is
     * handled without dereferencing the pointer.
     */
    ASSERT_TRUE(1);

    return 0;
}

static int test_valid_measurements_pass_validation(void)
{
    bms_measurements_t measurements = {
        .voltage = {
            .value = 48.0f,
            .status = BMS_MEAS_VALID
        },
        .current = {
            .value = 10.0f,
            .status = BMS_MEAS_VALID
        },
        .temperature = {
            .value = 25.0f,
            .status = BMS_MEAS_VALID
        }
    };

    ASSERT_EQ(0, bms_measurements_validate(&measurements));

    return 0;
}

static int test_invalid_voltage_fails_validation(void)
{
    bms_measurements_t measurements = {
        .voltage = {
            .value = 48.0f,
            .status = BMS_MEAS_INVALID
        },
        .current = {
            .value = 10.0f,
            .status = BMS_MEAS_VALID
        },
        .temperature = {
            .value = 25.0f,
            .status = BMS_MEAS_VALID
        }
    };

    ASSERT_EQ(-1, bms_measurements_validate(&measurements));

    return 0;
}

static int test_invalid_current_fails_validation(void)
{
    bms_measurements_t measurements = {
        .voltage = {
            .value = 48.0f,
            .status = BMS_MEAS_VALID
        },
        .current = {
            .value = 10.0f,
            .status = BMS_MEAS_INVALID
        },
        .temperature = {
            .value = 25.0f,
            .status = BMS_MEAS_VALID
        }
    };

    ASSERT_EQ(-1, bms_measurements_validate(&measurements));

    return 0;
}

static int test_invalid_temperature_fails_validation(void)
{
    bms_measurements_t measurements = {
        .voltage = {
            .value = 48.0f,
            .status = BMS_MEAS_VALID
        },
        .current = {
            .value = 10.0f,
            .status = BMS_MEAS_VALID
        },
        .temperature = {
            .value = 25.0f,
            .status = BMS_MEAS_INVALID
        }
    };

    ASSERT_EQ(-1, bms_measurements_validate(&measurements));

    return 0;
}

static int test_not_available_measurement_fails_validation(void)
{
    bms_measurements_t measurements = {
        .voltage = {
            .value = 48.0f,
            .status = BMS_MEAS_VALID
        },
        .current = {
            .value = 10.0f,
            .status = BMS_MEAS_NOT_AVAILABLE
        },
        .temperature = {
            .value = 25.0f,
            .status = BMS_MEAS_VALID
        }
    };

    ASSERT_EQ(-1, bms_measurements_validate(&measurements));

    return 0;
}

static int test_out_of_range_measurement_fails_validation(void)
{
    bms_measurements_t measurements = {
        .voltage = {
            .value = 60.0f,
            .status = BMS_MEAS_OUT_OF_RANGE
        },
        .current = {
            .value = 10.0f,
            .status = BMS_MEAS_VALID
        },
        .temperature = {
            .value = 25.0f,
            .status = BMS_MEAS_VALID
        }
    };

    ASSERT_EQ(-1, bms_measurements_validate(&measurements));

    return 0;
}

static int test_null_validation_fails(void)
{
    ASSERT_EQ(-1, bms_measurements_validate(NULL));

    return 0;
}

typedef int (*test_function_t)(void);

typedef struct
{
    const char *name;
    test_function_t function;
} test_case_t;

int main(void)
{
    const test_case_t tests[] = {
        {
            "init_sets_measurements_unavailable",
            test_init_sets_measurements_unavailable
        },
        {
            "init_null_pointer",
            test_init_null_pointer
        },
        {
            "valid_measurements_pass_validation",
            test_valid_measurements_pass_validation
        },
        {
            "invalid_voltage_fails_validation",
            test_invalid_voltage_fails_validation
        },
        {
            "invalid_current_fails_validation",
            test_invalid_current_fails_validation
        },
        {
            "invalid_temperature_fails_validation",
            test_invalid_temperature_fails_validation
        },
        {
            "not_available_measurement_fails_validation",
            test_not_available_measurement_fails_validation
        },
        {
            "out_of_range_measurement_fails_validation",
            test_out_of_range_measurement_fails_validation
        },
        {
            "null_validation_fails",
            test_null_validation_fails
        }
    };

    const size_t test_count =
        sizeof(tests) / sizeof(tests[0]);

    printf("[BMS UNIT] Running %zu tests...\n", test_count);

    for (size_t i = 0; i < test_count; ++i)
    {
        int result = tests[i].function();

        if (result != 0)
        {
            fprintf(stderr,
                    "[BMS UNIT] FAILED: %s\n",
                    tests[i].name);

            return 1;
        }

        printf("[PASS] %s\n", tests[i].name);
    }

    printf("[BMS UNIT] %d assertions passed.\n",
           tests_passed);

    return 0;
}