#include <assert.h>
#include <stdio.h>

#include "bms_manager.h"

/* Helper: create valid measurements */
static bms_measurements_t make_valid_measurements(float voltage,
                                                  float current,
                                                  float temperature)
{
    bms_measurements_t m;
    bms_measurements_init(&m);

    m.voltage.value = voltage;
    m.voltage.status = BMS_MEAS_VALID;

    m.current.value = current;
    m.current.status = BMS_MEAS_VALID;

    m.temperature.value = temperature;
    m.temperature.status = BMS_MEAS_VALID;

    return m;
}

/* Default limits: 40-54V, 20A, -20..60°C */
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

static void test_init(void)
{
    bms_manager_t manager;
    bms_limits_t limits = default_limits();

    bms_manager_init(&manager, &limits);

    assert(manager.status.state == BMS_STATE_INIT);
    assert(manager.status.fault == BMS_FAULT_NONE);

    assert(manager.protection == BMS_PROTECTION_INVALID_MEASUREMENT);

    assert(manager.measurements.voltage.status == BMS_MEAS_NOT_AVAILABLE);
    assert(manager.measurements.current.status == BMS_MEAS_NOT_AVAILABLE);
    assert(manager.measurements.temperature.status == BMS_MEAS_NOT_AVAILABLE);

    assert(manager.limits.min_voltage == limits.min_voltage);
    assert(manager.limits.max_voltage == limits.max_voltage);
    assert(manager.limits.max_current == limits.max_current);
    assert(manager.limits.min_temperature == limits.min_temperature);
    assert(manager.limits.max_temperature == limits.max_temperature);
}

static void test_update_normal(void)
{
    bms_manager_t manager;
    bms_limits_t limits = default_limits();
    bms_measurements_t measurements;

    bms_manager_init(&manager, &limits);

    measurements = make_valid_measurements(48.0f, 10.0f, 25.0f);
    bms_manager_update(&manager, &measurements);

    assert(manager.protection == BMS_PROTECTION_NORMAL);
    assert(manager.status.state == BMS_STATE_NORMAL);
    assert(manager.status.fault == BMS_FAULT_NONE);
}

static void test_update_overvoltage(void)
{
    bms_manager_t manager;
    bms_limits_t limits = default_limits();
    bms_measurements_t measurements;

    bms_manager_init(&manager, &limits);

    measurements = make_valid_measurements(55.0f, 10.0f, 25.0f);
    bms_manager_update(&manager, &measurements);

    assert(manager.protection == BMS_PROTECTION_OVER_VOLTAGE);
    assert(manager.status.state == BMS_STATE_FAULT);
    assert(manager.status.fault == BMS_FAULT_OVERVOLTAGE);
}

static void test_update_undervoltage(void)
{
    bms_manager_t manager;
    bms_limits_t limits = default_limits();
    bms_measurements_t measurements;

    bms_manager_init(&manager, &limits);

    measurements = make_valid_measurements(39.0f, 10.0f, 25.0f);
    bms_manager_update(&manager, &measurements);

    assert(manager.protection == BMS_PROTECTION_UNDER_VOLTAGE);
    assert(manager.status.state == BMS_STATE_FAULT);
    assert(manager.status.fault == BMS_FAULT_UNDERVOLTAGE);
}

static void test_update_overcurrent(void)
{
    bms_manager_t manager;
    bms_limits_t limits = default_limits();
    bms_measurements_t measurements;

    bms_manager_init(&manager, &limits);

    measurements = make_valid_measurements(48.0f, 21.0f, 25.0f);
    bms_manager_update(&manager, &measurements);

    assert(manager.protection == BMS_PROTECTION_OVER_CURRENT);
    assert(manager.status.state == BMS_STATE_FAULT);
    assert(manager.status.fault == BMS_FAULT_OVERCURRENT);
}

static void test_update_overtemperature(void)
{
    bms_manager_t manager;
    bms_limits_t limits = default_limits();
    bms_measurements_t measurements;

    bms_manager_init(&manager, &limits);

    measurements = make_valid_measurements(48.0f, 10.0f, 61.0f);
    bms_manager_update(&manager, &measurements);

    assert(manager.protection == BMS_PROTECTION_OVER_TEMPERATURE);
    assert(manager.status.state == BMS_STATE_FAULT);
    assert(manager.status.fault == BMS_FAULT_OVERTEMPERATURE);
}

static void test_update_undertemperature(void)
{
    bms_manager_t manager;
    bms_limits_t limits = default_limits();
    bms_measurements_t measurements;

    bms_manager_init(&manager, &limits);

    measurements = make_valid_measurements(48.0f, 10.0f, -21.0f);
    bms_manager_update(&manager, &measurements);

    assert(manager.protection == BMS_PROTECTION_UNDER_TEMPERATURE);
    assert(manager.status.state == BMS_STATE_FAULT);
    assert(manager.status.fault == BMS_FAULT_UNDERTEMPERATURE);
}

static void test_update_invalid_measurement(void)
{
    bms_manager_t manager;
    bms_limits_t limits = default_limits();
    bms_measurements_t measurements;

    bms_manager_init(&manager, &limits);

    measurements = make_valid_measurements(48.0f, 10.0f, 25.0f);
    measurements.voltage.status = BMS_MEAS_NOT_AVAILABLE;
    bms_manager_update(&manager, &measurements);

    assert(manager.protection == BMS_PROTECTION_INVALID_MEASUREMENT);
    assert(manager.status.state == BMS_STATE_FAULT);
    assert(manager.status.fault == BMS_FAULT_INVALID_MEASUREMENT);
}

static void test_null_arguments(void)
{
    bms_manager_t manager;
    bms_limits_t limits = default_limits();
    bms_measurements_t measurements = make_valid_measurements(48.0f, 10.0f, 25.0f);

    bms_manager_init(0, &limits);
    bms_manager_init(&manager, 0);
    bms_manager_update(0, &measurements);
    bms_manager_update(&manager, 0);
}

int main(void)
{
    printf("[BMS MANAGER UNIT] Running tests...\n");

    test_init();
    printf("[PASS] init\n");

    test_update_normal();
    printf("[PASS] update_normal\n");

    test_update_overvoltage();
    printf("[PASS] update_overvoltage\n");

    test_update_undervoltage();
    printf("[PASS] update_undervoltage\n");

    test_update_overcurrent();
    printf("[PASS] update_overcurrent\n");

    test_update_overtemperature();
    printf("[PASS] update_overtemperature\n");

    test_update_undertemperature();
    printf("[PASS] update_undertemperature\n");

    test_update_invalid_measurement();
    printf("[PASS] update_invalid_measurement\n");

    test_null_arguments();
    printf("[PASS] null_arguments\n");

    printf("[BMS MANAGER UNIT] All tests passed.\n");

    return 0;
}