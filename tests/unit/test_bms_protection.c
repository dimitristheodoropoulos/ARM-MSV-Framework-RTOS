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

/*
 * --------------------------------------------------------------------------
 * Existing protection tests
 * --------------------------------------------------------------------------
 */

static void test_basic_protection(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();

    m = valid_measurements();
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    m.voltage.value = 55.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_VOLTAGE);

    m = valid_measurements();
    m.voltage.value = 39.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_UNDER_VOLTAGE);

    m = valid_measurements();
    m.current.value = 21.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_CURRENT);

    m = valid_measurements();
    m.temperature.value = 61.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_TEMPERATURE);

    m = valid_measurements();
    m.temperature.value = -21.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_UNDER_TEMPERATURE);

    printf("[PASS] basic protection\n");
}

static void test_invalid_inputs(void)
{
    bms_measurements_t m = valid_measurements();
    bms_limits_t limits = default_limits();

    assert(bms_protection_evaluate(NULL, &limits) ==
           BMS_PROTECTION_INVALID_MEASUREMENT);

    assert(bms_protection_evaluate(&m, NULL) ==
           BMS_PROTECTION_INVALID_MEASUREMENT);

    m.voltage.status = BMS_MEAS_NOT_AVAILABLE;

    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_INVALID_MEASUREMENT);

    printf("[PASS] invalid inputs\n");
}

/*
 * --------------------------------------------------------------------------
 * Boundary tests
 * --------------------------------------------------------------------------
 */

static void test_voltage_boundaries(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();

    m = valid_measurements();

    /* Exact limits */
    m.voltage.value = 40.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    m.voltage.value = 54.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    /* Just inside */
    m.voltage.value = 40.0001f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    m.voltage.value = 53.9999f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    /* Just outside */
    m.voltage.value = 39.9999f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_UNDER_VOLTAGE);

    m.voltage.value = 54.0001f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_VOLTAGE);

    printf("[PASS] voltage boundaries\n");
}

static void test_current_boundaries(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();

    m = valid_measurements();

    /* Exact limit */
    m.current.value = 20.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    /* Just inside */
    m.current.value = 19.9999f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    /* Just outside */
    m.current.value = 20.0001f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_CURRENT);

    printf("[PASS] current boundaries\n");
}

static void test_temperature_boundaries(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();

    m = valid_measurements();

    /* Exact limits */
    m.temperature.value = -20.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    m.temperature.value = 60.0f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    /* Just inside */
    m.temperature.value = -19.9999f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    m.temperature.value = 59.9999f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_NORMAL);

    /* Just outside */
    m.temperature.value = -20.0001f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_UNDER_TEMPERATURE);

    m.temperature.value = 60.0001f;
    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_TEMPERATURE);

    printf("[PASS] temperature boundaries\n");
}

/*
 * --------------------------------------------------------------------------
 * BMS-REQ-049
 *
 * Existing deterministic-priority verification.
 *
 * The legacy protection API returns one primary protection status.
 * Multiple simultaneously active conditions are evaluated in the
 * established deterministic priority order:
 *
 *   over-voltage
 *   under-voltage
 *   over-current
 *   over-temperature
 *   under-temperature
 *
 * REQ-049 verifies this priority behaviour.
 * --------------------------------------------------------------------------
 */

static void test_req049_priority(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();

    /* Over-voltage + over-current -> over-voltage */
    m = valid_measurements();
    m.voltage.value = 55.0f;
    m.current.value = 21.0f;

    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_VOLTAGE);

    /* Over-voltage + over-temperature -> over-voltage */
    m = valid_measurements();
    m.voltage.value = 55.0f;
    m.temperature.value = 61.0f;

    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_VOLTAGE);

    /* Under-voltage + over-current -> under-voltage */
    m = valid_measurements();
    m.voltage.value = 39.0f;
    m.current.value = 21.0f;

    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_UNDER_VOLTAGE);

    /* Under-voltage + under-temperature -> under-voltage */
    m = valid_measurements();
    m.voltage.value = 39.0f;
    m.temperature.value = -21.0f;

    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_UNDER_VOLTAGE);

    /* Over-current + over-temperature -> over-current */
    m = valid_measurements();
    m.current.value = 21.0f;
    m.temperature.value = 61.0f;

    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_CURRENT);

    /* Over-current + under-temperature -> over-current */
    m = valid_measurements();
    m.current.value = 21.0f;
    m.temperature.value = -21.0f;

    assert(bms_protection_evaluate(&m, &limits) ==
           BMS_PROTECTION_OVER_CURRENT);

    /*
     * Over-temperature + under-temperature cannot coexist for a single
     * scalar temperature measurement.
     */

    printf("[PASS] BMS-REQ-049 deterministic priority\n");
}

/*
 * --------------------------------------------------------------------------
 * BMS-REQ-016
 *
 * Dedicated multi-fault representation.
 *
 * Unlike bms_protection_evaluate(), which returns one primary protection
 * status, bms_protection_evaluate_faults() returns a bitmask containing
 * every simultaneously active protection condition.
 * --------------------------------------------------------------------------
 */

static void test_req016_no_fault(void)
{
    bms_measurements_t m = valid_measurements();
    bms_limits_t limits = default_limits();
    bms_fault_mask_t faults;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults == BMS_FAULT_MASK_NONE);

    printf("[PASS] BMS-REQ-016 no-fault representation\n");
}

static void test_req016_single_faults(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();
    bms_fault_mask_t faults;

    /* Over-voltage */
    m = valid_measurements();
    m.voltage.value = 55.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults == BMS_FAULT_MASK_OVER_VOLTAGE);

    /* Under-voltage */
    m = valid_measurements();
    m.voltage.value = 39.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults == BMS_FAULT_MASK_UNDER_VOLTAGE);

    /* Over-current */
    m = valid_measurements();
    m.current.value = 21.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults == BMS_FAULT_MASK_OVER_CURRENT);

    /* Over-temperature */
    m = valid_measurements();
    m.temperature.value = 61.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults == BMS_FAULT_MASK_OVER_TEMPERATURE);

    /* Under-temperature */
    m = valid_measurements();
    m.temperature.value = -21.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults == BMS_FAULT_MASK_UNDER_TEMPERATURE);

    printf("[PASS] BMS-REQ-016 single-fault representation\n");
}

static void test_req016_voltage_current_multifault(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();
    bms_fault_mask_t faults;

    /*
     * Over-voltage + over-current.
     */
    m = valid_measurements();
    m.voltage.value = 55.0f;
    m.current.value = 21.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert((faults & BMS_FAULT_MASK_OVER_VOLTAGE) != 0u);
    assert((faults & BMS_FAULT_MASK_OVER_CURRENT) != 0u);
    assert((faults & BMS_FAULT_MASK_UNDER_VOLTAGE) == 0u);
    assert((faults & BMS_FAULT_MASK_OVER_TEMPERATURE) == 0u);
    assert((faults & BMS_FAULT_MASK_UNDER_TEMPERATURE) == 0u);

    assert(faults ==
           (BMS_FAULT_MASK_OVER_VOLTAGE |
            BMS_FAULT_MASK_OVER_CURRENT));

    /*
     * Under-voltage + over-current.
     */
    m = valid_measurements();
    m.voltage.value = 39.0f;
    m.current.value = 21.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert((faults & BMS_FAULT_MASK_UNDER_VOLTAGE) != 0u);
    assert((faults & BMS_FAULT_MASK_OVER_CURRENT) != 0u);

    assert(faults ==
           (BMS_FAULT_MASK_UNDER_VOLTAGE |
            BMS_FAULT_MASK_OVER_CURRENT));

    printf("[PASS] BMS-REQ-016 voltage/current multi-fault\n");
}

static void test_req016_voltage_temperature_multifault(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();
    bms_fault_mask_t faults;

    /*
     * Over-voltage + over-temperature.
     */
    m = valid_measurements();
    m.voltage.value = 55.0f;
    m.temperature.value = 61.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert((faults & BMS_FAULT_MASK_OVER_VOLTAGE) != 0u);
    assert((faults & BMS_FAULT_MASK_OVER_TEMPERATURE) != 0u);

    assert(faults ==
           (BMS_FAULT_MASK_OVER_VOLTAGE |
            BMS_FAULT_MASK_OVER_TEMPERATURE));

    /*
     * Over-voltage + under-temperature.
     */
    m = valid_measurements();
    m.voltage.value = 55.0f;
    m.temperature.value = -21.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert((faults & BMS_FAULT_MASK_OVER_VOLTAGE) != 0u);
    assert((faults & BMS_FAULT_MASK_UNDER_TEMPERATURE) != 0u);

    assert(faults ==
           (BMS_FAULT_MASK_OVER_VOLTAGE |
            BMS_FAULT_MASK_UNDER_TEMPERATURE));

    /*
     * Under-voltage + over-temperature.
     */
    m = valid_measurements();
    m.voltage.value = 39.0f;
    m.temperature.value = 61.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert((faults & BMS_FAULT_MASK_UNDER_VOLTAGE) != 0u);
    assert((faults & BMS_FAULT_MASK_OVER_TEMPERATURE) != 0u);

    assert(faults ==
           (BMS_FAULT_MASK_UNDER_VOLTAGE |
            BMS_FAULT_MASK_OVER_TEMPERATURE));

    /*
     * Under-voltage + under-temperature.
     */
    m = valid_measurements();
    m.voltage.value = 39.0f;
    m.temperature.value = -21.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert((faults & BMS_FAULT_MASK_UNDER_VOLTAGE) != 0u);
    assert((faults & BMS_FAULT_MASK_UNDER_TEMPERATURE) != 0u);

    assert(faults ==
           (BMS_FAULT_MASK_UNDER_VOLTAGE |
            BMS_FAULT_MASK_UNDER_TEMPERATURE));

    printf("[PASS] BMS-REQ-016 voltage/temperature multi-fault\n");
}

static void test_req016_current_temperature_multifault(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();
    bms_fault_mask_t faults;

    /*
     * Over-current + over-temperature.
     */
    m = valid_measurements();
    m.current.value = 21.0f;
    m.temperature.value = 61.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert((faults & BMS_FAULT_MASK_OVER_CURRENT) != 0u);
    assert((faults & BMS_FAULT_MASK_OVER_TEMPERATURE) != 0u);

    assert(faults ==
           (BMS_FAULT_MASK_OVER_CURRENT |
            BMS_FAULT_MASK_OVER_TEMPERATURE));

    /*
     * Over-current + under-temperature.
     */
    m = valid_measurements();
    m.current.value = 21.0f;
    m.temperature.value = -21.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert((faults & BMS_FAULT_MASK_OVER_CURRENT) != 0u);
    assert((faults & BMS_FAULT_MASK_UNDER_TEMPERATURE) != 0u);

    assert(faults ==
           (BMS_FAULT_MASK_OVER_CURRENT |
            BMS_FAULT_MASK_UNDER_TEMPERATURE));

    printf("[PASS] BMS-REQ-016 current/temperature multi-fault\n");
}

static void test_req016_three_simultaneous_faults(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();
    bms_fault_mask_t faults;

    /*
     * Over-voltage + over-current + over-temperature.
     */
    m = valid_measurements();
    m.voltage.value = 55.0f;
    m.current.value = 21.0f;
    m.temperature.value = 61.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert((faults & BMS_FAULT_MASK_OVER_VOLTAGE) != 0u);
    assert((faults & BMS_FAULT_MASK_OVER_CURRENT) != 0u);
    assert((faults & BMS_FAULT_MASK_OVER_TEMPERATURE) != 0u);

    assert(faults ==
           (BMS_FAULT_MASK_OVER_VOLTAGE |
            BMS_FAULT_MASK_OVER_CURRENT |
            BMS_FAULT_MASK_OVER_TEMPERATURE));

    /*
     * Under-voltage + over-current + under-temperature.
     */
    m = valid_measurements();
    m.voltage.value = 39.0f;
    m.current.value = 21.0f;
    m.temperature.value = -21.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert((faults & BMS_FAULT_MASK_UNDER_VOLTAGE) != 0u);
    assert((faults & BMS_FAULT_MASK_OVER_CURRENT) != 0u);
    assert((faults & BMS_FAULT_MASK_UNDER_TEMPERATURE) != 0u);

    assert(faults ==
           (BMS_FAULT_MASK_UNDER_VOLTAGE |
            BMS_FAULT_MASK_OVER_CURRENT |
            BMS_FAULT_MASK_UNDER_TEMPERATURE));

    printf("[PASS] BMS-REQ-016 three simultaneous faults\n");
}

static void test_req016_all_physically_possible_faults(void)
{
    bms_measurements_t m;
    bms_limits_t limits = default_limits();
    bms_fault_mask_t faults;

    /*
     * Maximum physically possible combination with a scalar voltage,
     * current and temperature model:
     *
     *   voltage can be either over OR under
     *   current can be over
     *   temperature can be either over OR under
     *
     * Therefore four independent combinations are physically possible:
     *
     *   OV + OC + OT
     *   OV + OC + UT
     *   UV + OC + OT
     *   UV + OC + UT
     *
     * OV + UV is impossible for one scalar voltage.
     * OT + UT is impossible for one scalar temperature.
     */

    /* OV + OC + OT */
    m = valid_measurements();
    m.voltage.value = 55.0f;
    m.current.value = 21.0f;
    m.temperature.value = 61.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults ==
           (BMS_FAULT_MASK_OVER_VOLTAGE |
            BMS_FAULT_MASK_OVER_CURRENT |
            BMS_FAULT_MASK_OVER_TEMPERATURE));

    /* OV + OC + UT */
    m = valid_measurements();
    m.voltage.value = 55.0f;
    m.current.value = 21.0f;
    m.temperature.value = -21.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults ==
           (BMS_FAULT_MASK_OVER_VOLTAGE |
            BMS_FAULT_MASK_OVER_CURRENT |
            BMS_FAULT_MASK_UNDER_TEMPERATURE));

    /* UV + OC + OT */
    m = valid_measurements();
    m.voltage.value = 39.0f;
    m.current.value = 21.0f;
    m.temperature.value = 61.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults ==
           (BMS_FAULT_MASK_UNDER_VOLTAGE |
            BMS_FAULT_MASK_OVER_CURRENT |
            BMS_FAULT_MASK_OVER_TEMPERATURE));

    /* UV + OC + UT */
    m = valid_measurements();
    m.voltage.value = 39.0f;
    m.current.value = 21.0f;
    m.temperature.value = -21.0f;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults ==
           (BMS_FAULT_MASK_UNDER_VOLTAGE |
            BMS_FAULT_MASK_OVER_CURRENT |
            BMS_FAULT_MASK_UNDER_TEMPERATURE));

    printf("[PASS] BMS-REQ-016 all physically possible multi-fault combinations\n");
}

static void test_req016_invalid_measurements(void)
{
    bms_measurements_t m = valid_measurements();
    bms_limits_t limits = default_limits();
    bms_fault_mask_t faults;

    /*
     * Invalid measurements must not be interpreted as protection faults.
     * The multi-fault API returns NONE because it has no separate invalid
     * measurement bit; invalid input remains represented by the existing
     * BMS_PROTECTION_INVALID_MEASUREMENT status API.
     */
    m.voltage.status = BMS_MEAS_INVALID;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults == BMS_FAULT_MASK_NONE);

    m = valid_measurements();
    m.current.status = BMS_MEAS_NOT_AVAILABLE;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults == BMS_FAULT_MASK_NONE);

    m = valid_measurements();
    m.temperature.status = BMS_MEAS_INVALID;

    faults = bms_protection_evaluate_faults(&m, &limits);

    assert(faults == BMS_FAULT_MASK_NONE);

    printf("[PASS] BMS-REQ-016 invalid measurements\n");
}

static void test_req016_null_arguments(void)
{
    bms_measurements_t m = valid_measurements();
    bms_limits_t limits = default_limits();
    bms_fault_mask_t faults;

    faults = bms_protection_evaluate_faults(NULL, &limits);
    assert(faults == BMS_FAULT_MASK_NONE);

    faults = bms_protection_evaluate_faults(&m, NULL);
    assert(faults == BMS_FAULT_MASK_NONE);

    faults = bms_protection_evaluate_faults(NULL, NULL);
    assert(faults == BMS_FAULT_MASK_NONE);

    printf("[PASS] BMS-REQ-016 null arguments\n");
}

/*
 * --------------------------------------------------------------------------
 * Main
 * --------------------------------------------------------------------------
 */

int main(void)
{
    printf("[BMS PROTECTION UNIT] Running extended protection tests...\n");

    /*
     * Existing protection verification.
     */
    test_basic_protection();
    test_invalid_inputs();

    test_voltage_boundaries();
    test_current_boundaries();
    test_temperature_boundaries();

    /*
     * Existing deterministic-priority verification.
     */
    test_req049_priority();

    /*
     * Dedicated BMS-REQ-016 multi-fault representation verification.
     */
    test_req016_no_fault();
    test_req016_single_faults();
    test_req016_voltage_current_multifault();
    test_req016_voltage_temperature_multifault();
    test_req016_current_temperature_multifault();
    test_req016_three_simultaneous_faults();
    test_req016_all_physically_possible_faults();
    test_req016_invalid_measurements();
    test_req016_null_arguments();

    printf("[BMS PROTECTION UNIT] All tests passed.\n");

    return 0;
}
