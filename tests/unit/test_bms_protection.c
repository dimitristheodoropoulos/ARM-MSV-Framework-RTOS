#include <assert.h>
#include <stdio.h>

#include "bms_measurements.h"
#include "bms_protection.h"

static bms_measurements_t valid_measurements(void)
{
    bms_measurements_t m;

    bms_measurements_init(&m);

    m.voltage.value = 48.0f;
    m.voltage.status = BMS_MEAS_VALID;

    m.current.value = 10.0f;
    m.current.status = BMS_MEAS_VALID;

    m.temperature.value = 25.0f;
    m.temperature.status = BMS_MEAS_VALID;

    return m;
}

static bms_limits_t default_limits(void)
{
    bms_limits_t limits;

    limits.min_voltage = 40.0f;
    limits.max_voltage = 54.0f;
    limits.max_current = 20.0f;
    limits.min_temperature = -20.0f;
    limits.max_temperature = 60.0f;

    return limits;
}

int main(void)
{
    bms_measurements_t measurements;
    bms_limits_t limits;

    printf("[BMS PROTECTION UNIT] Running tests...\n");

    measurements = valid_measurements();
    limits = default_limits();

    assert(
        bms_protection_evaluate(&measurements, &limits)
        == BMS_PROTECTION_NORMAL
    );

    measurements.voltage.value = 55.0f;

    assert(
        bms_protection_evaluate(&measurements, &limits)
        == BMS_PROTECTION_OVER_VOLTAGE
    );

    measurements = valid_measurements();
    measurements.voltage.value = 39.0f;

    assert(
        bms_protection_evaluate(&measurements, &limits)
        == BMS_PROTECTION_UNDER_VOLTAGE
    );

    measurements = valid_measurements();
    measurements.current.value = 21.0f;

    assert(
        bms_protection_evaluate(&measurements, &limits)
        == BMS_PROTECTION_OVER_CURRENT
    );

    measurements = valid_measurements();
    measurements.temperature.value = 61.0f;

    assert(
        bms_protection_evaluate(&measurements, &limits)
        == BMS_PROTECTION_OVER_TEMPERATURE
    );

    measurements = valid_measurements();
    measurements.temperature.value = -21.0f;

    assert(
        bms_protection_evaluate(&measurements, &limits)
        == BMS_PROTECTION_UNDER_TEMPERATURE
    );

    assert(
        bms_protection_evaluate(NULL, &limits)
        == BMS_PROTECTION_INVALID_MEASUREMENT
    );

    measurements = valid_measurements();

    assert(
        bms_protection_evaluate(&measurements, NULL)
        == BMS_PROTECTION_INVALID_MEASUREMENT
    );

    measurements.voltage.status = BMS_MEAS_NOT_AVAILABLE;

    assert(
        bms_protection_evaluate(&measurements, &limits)
        == BMS_PROTECTION_INVALID_MEASUREMENT
    );

    printf("[BMS PROTECTION UNIT] All tests passed.\n");

    return 0;
}