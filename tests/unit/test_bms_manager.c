#include <assert.h>
#include <stdio.h>

#include "bms_manager.h"

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
    /* κλπ. */
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

/* Νέα δοκιμή: ακριβώς στα όρια */
static void test_update_exact_boundaries(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();
    bms_manager_init(&mgr, &limits);

    /* Τάση ακριβώς στα όρια */
    bms_measurements_t m = make_valid_measurements(40.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);

    m = make_valid_measurements(54.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    /* Ρεύμα ακριβώς */
    m = make_valid_measurements(48.0f, 20.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    /* Θερμοκρασία ακριβώς */
    m = make_valid_measurements(48.0f, 10.0f, -20.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);

    m = make_valid_measurements(48.0f, 10.0f, 60.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);
}

/* Νέα δοκιμή: λίγο εντός / εκτός */
static void test_update_just_inside_outside(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();
    bms_manager_init(&mgr, &limits);

    /* just inside */
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

    /* just outside */
    m = make_valid_measurements(39.9999f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_UNDER_VOLTAGE);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_UNDERVOLTAGE);

    m = make_valid_measurements(54.0001f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_OVER_VOLTAGE);

    m = make_valid_measurements(48.0f, 20.0001f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_OVER_CURRENT);

    m = make_valid_measurements(48.0f, 10.0f, -20.0001f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_UNDER_TEMPERATURE);

    m = make_valid_measurements(48.0f, 10.0f, 60.0001f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_OVER_TEMPERATURE);
}

/* Νέα δοκιμή: μετάβαση FAULT → NORMAL μέσω manager */
static void test_update_transition_fault_to_normal(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();
    bms_manager_init(&mgr, &limits);

    /* Αρχικά NORMAL */
    bms_measurements_t m = make_valid_measurements(48.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);

    /* Προκαλούμε OVERVOLTAGE */
    m = make_valid_measurements(55.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_OVER_VOLTAGE);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_OVERVOLTAGE);

    /* Επιστροφή σε NORMAL */
    m = make_valid_measurements(48.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);
    assert(mgr.status.fault == BMS_FAULT_NONE);
}


/* BMS-REQ-050 — end-to-end BMS module integration */
static void test_integration_measurement_to_state(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();

    bms_manager_init(&mgr, &limits);

    /* NORMAL path: measurements → protection → state */
    bms_measurements_t m = make_valid_measurements(48.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);

    assert(mgr.measurements.voltage.value == 48.0f);
    assert(mgr.measurements.current.value == 10.0f);
    assert(mgr.measurements.temperature.value == 25.0f);
    assert(mgr.measurements.voltage.status == BMS_MEAS_VALID);
    assert(mgr.measurements.current.status == BMS_MEAS_VALID);
    assert(mgr.measurements.temperature.status == BMS_MEAS_VALID);

    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);
    assert(mgr.status.fault == BMS_FAULT_NONE);

    /* Fault propagation: protection result → state/fault */
    m.voltage.value = 55.0f;
    bms_manager_update(&mgr, &m);

    assert(mgr.protection == BMS_PROTECTION_OVER_VOLTAGE);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_OVERVOLTAGE);

    /* Invalid measurement propagation */
    m = make_valid_measurements(48.0f, 10.0f, 25.0f);
    m.temperature.status = BMS_MEAS_INVALID;
    bms_manager_update(&mgr, &m);

    assert(mgr.protection == BMS_PROTECTION_INVALID_MEASUREMENT);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_INVALID_MEASUREMENT);

    /* Recovery: valid measurements restore NORMAL state */
    m = make_valid_measurements(48.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);

    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);
    assert(mgr.status.fault == BMS_FAULT_NONE);

    printf("[PASS] BMS-REQ-050 end-to-end module integration\n");
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

    test_integration_measurement_to_state();

    test_null_arguments();
    printf("[PASS] null_arguments\n");

    printf("[BMS MANAGER UNIT] All tests passed.\n");
    return 0;
}
