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

/*
 * ========================================================
 *  BMS-REQ-016: Multi‑fault integration tests
 * ========================================================
 */

static void test_req016_multifault_manager_integration(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();

    bms_manager_init(&mgr, &limits);

    /*
     * BMS-REQ-016:
     * Multiple protection conditions must be represented
     * simultaneously at manager level.
     *
     * Active conditions:
     *   - over-voltage
     *   - over-current
     *   - over-temperature
     *
     * The manager must preserve all three in fault_mask,
     * while the legacy primary protection API must retain
     * deterministic priority and select over-voltage.
     */
    bms_measurements_t m =
        make_valid_measurements(55.0f, 21.0f, 61.0f);

    bms_manager_update(&mgr, &m);

    assert((mgr.fault_mask & BMS_FAULT_MASK_OVER_VOLTAGE) != 0u);
    assert((mgr.fault_mask & BMS_FAULT_MASK_OVER_CURRENT) != 0u);
    assert((mgr.fault_mask & BMS_FAULT_MASK_OVER_TEMPERATURE) != 0u);

    assert(mgr.fault_mask ==
           (BMS_FAULT_MASK_OVER_VOLTAGE |
            BMS_FAULT_MASK_OVER_CURRENT |
            BMS_FAULT_MASK_OVER_TEMPERATURE));

    /* Primary protection remains deterministic. */
    assert(mgr.protection == BMS_PROTECTION_OVER_VOLTAGE);

    /* Existing state/fault pipeline remains deterministic. */
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_OVERVOLTAGE);

    /*
     * Recovery must clear the multi-fault representation as well
     * as the primary protection/state result.
     */
    m = make_valid_measurements(48.0f, 10.0f, 25.0f);
    bms_manager_update(&mgr, &m);

    assert(mgr.fault_mask == BMS_FAULT_MASK_NONE);
    assert(mgr.protection == BMS_PROTECTION_NORMAL);
    assert(mgr.status.state == BMS_STATE_NORMAL);
    assert(mgr.status.fault == BMS_FAULT_NONE);

    printf("[PASS] BMS-REQ-016 manager multi-fault integration\n");
}

static void test_req016_manager_multiple_fault_combinations(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();

    bms_manager_init(&mgr, &limits);

    /*
     * Under-voltage + under-temperature.
     */
    bms_measurements_t m =
        make_valid_measurements(39.0f, 10.0f, -21.0f);

    bms_manager_update(&mgr, &m);

    assert(mgr.fault_mask ==
           (BMS_FAULT_MASK_UNDER_VOLTAGE |
            BMS_FAULT_MASK_UNDER_TEMPERATURE));

    assert((mgr.fault_mask & BMS_FAULT_MASK_UNDER_VOLTAGE) != 0u);
    assert((mgr.fault_mask & BMS_FAULT_MASK_UNDER_TEMPERATURE) != 0u);

    /*
     * Primary protection remains under-voltage because
     * voltage has higher priority than temperature.
     */
    assert(mgr.protection == BMS_PROTECTION_UNDER_VOLTAGE);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_UNDERVOLTAGE);

    /*
     * Current + under-temperature.
     */
    m = make_valid_measurements(48.0f, 21.0f, -21.0f);

    bms_manager_update(&mgr, &m);

    assert(mgr.fault_mask ==
           (BMS_FAULT_MASK_OVER_CURRENT |
            BMS_FAULT_MASK_UNDER_TEMPERATURE));

    assert((mgr.fault_mask & BMS_FAULT_MASK_OVER_CURRENT) != 0u);
    assert((mgr.fault_mask & BMS_FAULT_MASK_UNDER_TEMPERATURE) != 0u);

    assert(mgr.protection == BMS_PROTECTION_OVER_CURRENT);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_OVERCURRENT);

    printf("[PASS] BMS-REQ-016 manager fault combinations\n");
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


static void test_req027_manager_accepts_valid_limits(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();

    bms_manager_init(&mgr, &limits);

    assert(mgr.limits_valid == 1);
}

static void test_req028_manager_rejects_invalid_limits(void)
{
    bms_manager_t mgr;
    bms_limits_t limits = default_limits();
    limits.min_voltage = limits.max_voltage;

    bms_manager_init(&mgr, &limits);

    assert(mgr.limits_valid == 0);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_INVALID_MEASUREMENT);
    assert(mgr.fault_mask == BMS_FAULT_MASK_NONE);

    bms_measurements_t measurements =
        make_valid_measurements(48.0f, 5.0f, 25.0f);

    bms_manager_update(&mgr, &measurements);

    /*
     * Invalid configuration must remain rejected; the manager must
     * not execute the normal protection pipeline.
     */
    assert(mgr.limits_valid == 0);
    assert(mgr.status.state == BMS_STATE_FAULT);
    assert(mgr.status.fault == BMS_FAULT_INVALID_MEASUREMENT);
    assert(mgr.fault_mask == BMS_FAULT_MASK_NONE);
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

    /* REQ-016 tests */
    test_req016_multifault_manager_integration();
    test_req016_manager_multiple_fault_combinations();

    test_null_arguments();
    printf("[PASS] null_arguments\n");

    test_req027_manager_accepts_valid_limits();
    printf("[PASS] BMS-REQ-027 manager accepts valid limits\n");

    test_req028_manager_rejects_invalid_limits();
    printf("[PASS] BMS-REQ-028 manager rejects invalid limits\n");

    printf("[BMS MANAGER UNIT] All tests passed.\n");
    return 0;
}