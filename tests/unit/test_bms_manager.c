#include <assert.h>
#include <stdio.h>

#include "bms_manager.h"
#include "bms_limits.h"

static bms_measurements_t make_valid_measurements(float v, float i, float t)
{
    bms_measurements_t m;
    bms_measurements_init(&m);
    m.voltage.value = v;
    m.voltage.status = BMS_MEAS_VALID;
    m.current.value = i;
    m.current.status = BMS_MEAS_VALID;
    m.temperature.value = t;
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

static void test_init(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();
    bms_manager_init(&mgr, &limits);

    assert(mgr.status.state == BMS_STATE_INIT);
    assert(mgr.status.fault == BMS_FAULT_NONE);
    assert(mgr.protection == BMS_PROTECTION_INVALID_MEASUREMENT);
    assert(mgr.measurements.voltage.status == BMS_MEAS_NOT_AVAILABLE);
    assert(mgr.limits.min_voltage == limits.min_voltage);
}

static void test_update_normal(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();
    bms_manager_init(&mgr, &limits);

    bms_measurements_t m = make_valid_measurements(48.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);

    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);
    assert(mgr.status.fault == BMS_FAULT_NONE);
}

static void test_update_exact_boundaries(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();
    bms_manager_init(&mgr, &limits);

    bms_measurements_t m = make_valid_measurements(40.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);

    m = make_valid_measurements(54.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    m = make_valid_measurements(48.0f, 20.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    m = make_valid_measurements(48.0f, 10.0f, -20.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    m = make_valid_measurements(48.0f, 10.0f, 60.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);
}

static void test_update_just_inside_outside(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();
    bms_manager_init(&mgr, &limits);

    bms_measurements_t m = make_valid_measurements(40.0001f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    m = make_valid_measurements(53.9999f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    m = make_valid_measurements(48.0f, 19.9999f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    m = make_valid_measurements(48.0f, 10.0f, -19.9999f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    m = make_valid_measurements(48.0f, 10.0f, 59.9999f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    m = make_valid_measurements(39.9999f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_UNDER_VOLTAGE);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_UNDERVOLTAGE);

    m = make_valid_measurements(54.0001f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_OVER_VOLTAGE);

    /* Positive current just outside */
    m = make_valid_measurements(48.0f, 20.0001f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_OVER_CURRENT);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_OVERCURRENT);

    /* Negative current boundary tests */
    m = make_valid_measurements(48.0f, -20.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);
    assert(mgr.status.fault == BMS_FAULT_NONE);

    m = make_valid_measurements(48.0f, -19.9999f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    m = make_valid_measurements(48.0f, -20.0001f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_OVER_CURRENT);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_OVERCURRENT);

    m = make_valid_measurements(48.0f, 10.0f, -20.0001f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_UNDER_TEMPERATURE);

    m = make_valid_measurements(48.0f, 10.0f, 60.0001f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_OVER_TEMPERATURE);
}

static void test_update_transition_fault_to_normal(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();
    bms_manager_init(&mgr, &limits);

    bms_measurements_t m = make_valid_measurements(48.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);

    m = make_valid_measurements(55.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_OVER_VOLTAGE);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_OVERVOLTAGE);

    m = make_valid_measurements(48.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);
    assert(mgr.status.fault == BMS_FAULT_NONE);
}

/* REQ-028: invalid limits configuration */
static void test_invalid_limits_configuration(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();

    /* Invalid: min_voltage >= max_voltage */
    limits.min_voltage = 54.0f;
    limits.max_voltage = 40.0f;

    assert(bms_limits_validate(&limits) == -1);

    bms_manager_init(&mgr, &limits);

    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_INVALID_CONFIGURATION);
}

/* Configuration latch test */
static void test_invalid_configuration_is_latched(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();

    /* Invalid: min_voltage >= max_voltage */
    limits.min_voltage = 54.0f;
    limits.max_voltage = 40.0f;

    bms_manager_init(&mgr, &limits);

    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_INVALID_CONFIGURATION);

    /* Valid measurements must NOT clear the invalid-configuration fault */
    bms_measurements_t m =
        make_valid_measurements(48.0f, 10.0f, 25.0f);

    bms_manager_update(&mgr, &m);

    /* Still FAULT / INVALID_CONFIGURATION */
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_INVALID_CONFIGURATION);
}

static void test_null_arguments(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();
    bms_measurements_t m = make_valid_measurements(48.0f, 10.0f, 25.0f);

    bms_manager_init(0, &limits);
    bms_manager_init(&mgr, 0);
    bms_manager_update(0, &m);
    bms_manager_update(&mgr, 0);
}

int main(void)
{
    printf("[BMS MANAGER UNIT] Running extended end-to-end tests...\n");

    test_init();
    printf("[PASS] init\n");

    test_update_normal();
    printf("[PASS] update_normal\n");

    test_update_exact_boundaries();
    printf("[PASS] exact boundaries\n");

    test_update_just_inside_outside();
    printf("[PASS] just inside/outside\n");

    test_update_transition_fault_to_normal();
    printf("[PASS] transition fault → normal\n");

    test_invalid_limits_configuration();
    printf("[PASS] invalid_limits_configuration\n");

    test_invalid_configuration_is_latched();
    printf("[PASS] invalid_configuration_is_latched\n");

    test_null_arguments();
    printf("[PASS] null_arguments\n");

    printf("[BMS MANAGER UNIT] All tests passed.\n");
    return 0;
}
