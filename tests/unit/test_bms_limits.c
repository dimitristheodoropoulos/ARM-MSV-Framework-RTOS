#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "bms_limits.h"

static bms_limits_t valid_limits(void)
{
    bms_limits_t limits;

    limits.min_voltage = 40.0f;
    limits.max_voltage = 54.0f;
    limits.max_current = 20.0f;
    limits.min_temperature = -20.0f;
    limits.max_temperature = 60.0f;

    return limits;
}

static void test_valid_configuration(void)
{
    bms_limits_t limits = valid_limits();

    assert(bms_limits_validate(&limits) == 0);
}

static void test_null_configuration(void)
{
    assert(bms_limits_validate(NULL) == -1);
}

static void test_invalid_voltage_equal(void)
{
    bms_limits_t limits = valid_limits();

    limits.min_voltage = 54.0f;
    limits.max_voltage = 54.0f;

    assert(bms_limits_validate(&limits) == -1);
}

static void test_invalid_voltage_reversed(void)
{
    bms_limits_t limits = valid_limits();

    limits.min_voltage = 55.0f;
    limits.max_voltage = 54.0f;

    assert(bms_limits_validate(&limits) == -1);
}

static void test_invalid_temperature_equal(void)
{
    bms_limits_t limits = valid_limits();

    limits.min_temperature = 60.0f;
    limits.max_temperature = 60.0f;

    assert(bms_limits_validate(&limits) == -1);
}

static void test_invalid_temperature_reversed(void)
{
    bms_limits_t limits = valid_limits();

    limits.min_temperature = 61.0f;
    limits.max_temperature = 60.0f;

    assert(bms_limits_validate(&limits) == -1);
}

static void test_invalid_current_zero(void)
{
    bms_limits_t limits = valid_limits();

    limits.max_current = 0.0f;

    assert(bms_limits_validate(&limits) == -1);
}

static void test_invalid_current_negative(void)
{
    bms_limits_t limits = valid_limits();

    limits.max_current = -1.0f;

    assert(bms_limits_validate(&limits) == -1);
}

static void test_nan_rejected(void)
{
    bms_limits_t limits = valid_limits();

    limits.max_voltage = NAN;
    assert(bms_limits_validate(&limits) == -1);

    limits = valid_limits();
    limits.min_voltage = NAN;
    assert(bms_limits_validate(&limits) == -1);

    limits = valid_limits();
    limits.max_current = NAN;
    assert(bms_limits_validate(&limits) == -1);

    limits = valid_limits();
    limits.min_temperature = NAN;
    assert(bms_limits_validate(&limits) == -1);

    limits = valid_limits();
    limits.max_temperature = NAN;
    assert(bms_limits_validate(&limits) == -1);
}

static void test_infinity_rejected(void)
{
    bms_limits_t limits = valid_limits();

    limits.max_voltage = INFINITY;
    assert(bms_limits_validate(&limits) == -1);

    limits = valid_limits();
    limits.min_voltage = -INFINITY;
    assert(bms_limits_validate(&limits) == -1);

    limits = valid_limits();
    limits.max_current = INFINITY;
    assert(bms_limits_validate(&limits) == -1);

    limits = valid_limits();
    limits.min_temperature = -INFINITY;
    assert(bms_limits_validate(&limits) == -1);

    limits = valid_limits();
    limits.max_temperature = INFINITY;
    assert(bms_limits_validate(&limits) == -1);
}

int main(void)
{
    printf("[BMS LIMITS UNIT] Running validation tests...\n");

    test_valid_configuration();
    printf("[PASS] valid configuration\n");

    test_null_configuration();
    printf("[PASS] null configuration\n");

    test_invalid_voltage_equal();
    printf("[PASS] voltage equality rejected\n");

    test_invalid_voltage_reversed();
    printf("[PASS] voltage reversal rejected\n");

    test_invalid_temperature_equal();
    printf("[PASS] temperature equality rejected\n");

    test_invalid_temperature_reversed();
    printf("[PASS] temperature reversal rejected\n");

    test_invalid_current_zero();
    printf("[PASS] zero current rejected\n");

    test_invalid_current_negative();
    printf("[PASS] negative current rejected\n");

    test_nan_rejected();
    printf("[PASS] NaN rejected\n");

    test_infinity_rejected();
    printf("[PASS] infinity rejected\n");

    printf("[BMS LIMITS UNIT] All validation tests passed.\n");

    return 0;
}
